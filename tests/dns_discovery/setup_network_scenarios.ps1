# setup_network_scenarios.ps1
# Creates virtual network adapters for DNS discovery testing.
#
# Uses Hyper-V internal switch (available on GitHub runners with Docker)
# to create a real Ethernet-type adapter visible to GetAdaptersAddresses.
#
# Usage: .\setup_network_scenarios.ps1 -Scenario <name>
# Requires: Administrator privileges
# Exit code: 0 = success, 1 = verification failed

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("loopback", "disable", "metric", "broadcast", "no_dns", "multi_dns", "flapping_setup", "ipv6", "dedup", "stale_vpn", "apipa_unicast", "dns_no_ip", "teardown")]
    [string]$Scenario
)

$ErrorActionPreference = "Stop"

$SwitchName = "PNTestSwitch"
$AdapterAlias = "vEthernet ($SwitchName)"


# ──────────────────────────────────────────────
#  Verification helpers
# ──────────────────────────────────────────────

function Verify-AdapterStatus {
    param(
        [string]$Name,
        [string]$ExpectedStatus  # "Up" or "Disabled"
    )

    $adapter = Get-NetAdapter -Name $Name -ErrorAction SilentlyContinue
    if (-not $adapter) {
        Write-Host "  [VERIFY FAIL] Adapter '$Name' not found" -ForegroundColor Red
        return $false
    }

    if ($adapter.Status -ne $ExpectedStatus) {
        Write-Host "  [VERIFY FAIL] Adapter '$Name' status is '$($adapter.Status)', expected '$ExpectedStatus'" -ForegroundColor Red
        return $false
    }

    Write-Host "  [VERIFY OK] Adapter '$Name' status: $($adapter.Status)" -ForegroundColor Green
    return $true
}

function Verify-DnsServers {
    param(
        [string]$InterfaceAlias,
        [string[]]$ExpectedDns    # empty array = expect no DNS
    )

    $dns = Get-DnsClientServerAddress -InterfaceAlias $InterfaceAlias -AddressFamily IPv4 -ErrorAction SilentlyContinue
    $actual = @()
    if ($dns -and $dns.ServerAddresses) {
        $actual = @($dns.ServerAddresses)
    }

    if ($ExpectedDns.Count -eq 0) {
        if ($actual.Count -eq 0) {
            Write-Host "  [VERIFY OK] '$InterfaceAlias' DNS: (none, as expected)" -ForegroundColor Green
            return $true
        } else {
            Write-Host "  [VERIFY FAIL] '$InterfaceAlias' DNS: expected none, got: $($actual -join ', ')" -ForegroundColor Red
            return $false
        }
    }

    $missing = @()
    foreach ($expected in $ExpectedDns) {
        if ($actual -notcontains $expected) {
            $missing += $expected
        }
    }

    if ($missing.Count -gt 0) {
        Write-Host "  [VERIFY FAIL] '$InterfaceAlias' DNS missing: $($missing -join ', ') (actual: $($actual -join ', '))" -ForegroundColor Red
        return $false
    }

    Write-Host "  [VERIFY OK] '$InterfaceAlias' DNS: $($actual -join ', ')" -ForegroundColor Green
    return $true
}

function Verify-IpAddress {
    param(
        [string]$InterfaceAlias,
        [string]$ExpectedIp
    )

    $ip = Get-NetIPAddress -InterfaceAlias $InterfaceAlias -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Where-Object { $_.IPAddress -eq $ExpectedIp }

    if (-not $ip) {
        $all = Get-NetIPAddress -InterfaceAlias $InterfaceAlias -AddressFamily IPv4 -ErrorAction SilentlyContinue
        $actual = if ($all) { ($all | ForEach-Object { $_.IPAddress }) -join ', ' } else { "(none)" }
        Write-Host "  [VERIFY FAIL] '$InterfaceAlias' IP: expected $ExpectedIp, got: $actual" -ForegroundColor Red
        return $false
    }

    Write-Host "  [VERIFY OK] '$InterfaceAlias' IP: $ExpectedIp" -ForegroundColor Green
    return $true
}

function Verify-NoValidIpv4Address {
    param(
        [string]$InterfaceAlias
    )

    $allIp = Get-NetIPAddress -InterfaceAlias $InterfaceAlias -AddressFamily IPv4 -ErrorAction SilentlyContinue
    $validIp = @()
    if ($allIp) {
        $validIp = @($allIp | Where-Object {
            -not $_.IPAddress.StartsWith("169.254.") -and
            -not $_.IPAddress.StartsWith("127.") -and
            $_.IPAddress -ne "0.0.0.0"
        })
    }

    if ($validIp.Count -gt 0) {
        $actual = ($validIp | ForEach-Object { $_.IPAddress }) -join ', '
        Write-Host "  [VERIFY FAIL] '$InterfaceAlias' expected no valid IPv4 unicast, got: $actual" -ForegroundColor Red
        return $false
    }

    $raw = if ($allIp) { ($allIp | ForEach-Object { $_.IPAddress }) -join ', ' } else { "(none)" }
    Write-Host "  [VERIFY OK] '$InterfaceAlias' has no valid IPv4 unicast (raw: $raw)" -ForegroundColor Green
    return $true
}

function Verify-Metric {
    param(
        [string]$InterfaceAlias,
        [int]$ExpectedMetric
    )

    $iface = Get-NetIPInterface -InterfaceAlias $InterfaceAlias -AddressFamily IPv4 -ErrorAction SilentlyContinue
    if (-not $iface) {
        Write-Host "  [VERIFY FAIL] '$InterfaceAlias' interface not found" -ForegroundColor Red
        return $false
    }

    if ($iface.InterfaceMetric -ne $ExpectedMetric) {
        Write-Host "  [VERIFY FAIL] '$InterfaceAlias' metric: expected $ExpectedMetric, got $($iface.InterfaceMetric)" -ForegroundColor Red
        return $false
    }

    Write-Host "  [VERIFY OK] '$InterfaceAlias' metric: $($iface.InterfaceMetric)" -ForegroundColor Green
    return $true
}


# ──────────────────────────────────────────────
#  Adapter management via Hyper-V internal switch
# ──────────────────────────────────────────────

function Ensure-TestAdapter {
    Write-Host "  Creating Hyper-V internal switch '$SwitchName'..."

    # Check if already exists
    $existing = Get-VMSwitch -Name $SwitchName -ErrorAction SilentlyContinue
    if ($existing) {
        Write-Host "  Switch already exists"
    } else {
        New-VMSwitch -Name $SwitchName -SwitchType Internal | Out-Null
        Start-Sleep -Seconds 3
    }

    # Verify the host adapter appeared
    $adapter = Get-NetAdapter -Name $AdapterAlias -ErrorAction SilentlyContinue
    if (-not $adapter) {
        Write-Host "  [ERROR] Adapter '$AdapterAlias' did not appear after switch creation" -ForegroundColor Red
        Get-NetAdapter | Format-Table Name, InterfaceDescription, Status -AutoSize
        return $false
    }

    # Enable if needed
    if ($adapter.Status -ne "Up") {
        Enable-NetAdapter -Name $AdapterAlias -Confirm:$false
        Start-Sleep -Seconds 2
    }

    # Clean existing IP config
    Remove-NetIPAddress -InterfaceAlias $AdapterAlias -Confirm:$false -ErrorAction SilentlyContinue
    Remove-NetRoute -InterfaceAlias $AdapterAlias -Confirm:$false -ErrorAction SilentlyContinue

    Write-Host "  Adapter '$AdapterAlias' ready (status: $((Get-NetAdapter -Name $AdapterAlias).Status))"
    return $true
}


# ──────────────────────────────────────────────
#  Scenario setup + verification
# ──────────────────────────────────────────────

function Setup-Loopback {
    Write-Host "Setting up test adapter with fake DNS..."

    if (-not (Ensure-TestAdapter)) { exit 1 }

    New-NetIPAddress -InterfaceAlias $AdapterAlias -IPAddress "192.168.200.1" -PrefixLength 24 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ServerAddresses "10.255.255.1"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-IpAddress $AdapterAlias "192.168.200.1") -and
          (Verify-DnsServers $AdapterAlias @("10.255.255.1"))
    if (-not $ok) { exit 1 }
}

function Setup-Disable {
    Write-Host "Disabling test adapter..."

    $adapter = Get-NetAdapter -Name $AdapterAlias -ErrorAction SilentlyContinue
    if (-not $adapter) {
        Write-Warning "Test adapter not found - skipping"
        exit 1
    }

    Disable-NetAdapter -Name $AdapterAlias -Confirm:$false
    Start-Sleep -Seconds 2

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = Verify-AdapterStatus $AdapterAlias "Disabled"
    if (-not $ok) { exit 1 }
}

function Setup-Metric {
    Write-Host "Setting high metric on test adapter..."

    $adapter = Get-NetAdapter -Name $AdapterAlias -ErrorAction SilentlyContinue
    if (-not $adapter) {
        Write-Warning "Test adapter not found - skipping"
        exit 1
    }

    if ($adapter.Status -ne "Up") {
        Enable-NetAdapter -Name $AdapterAlias -Confirm:$false
        Start-Sleep -Seconds 2
    }

    Set-NetIPInterface -InterfaceAlias $AdapterAlias -InterfaceMetric 9999 -ErrorAction SilentlyContinue

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-Metric $AdapterAlias 9999)
    if (-not $ok) { exit 1 }
}

function Setup-Broadcast {
    Write-Host "Setting up test adapter with broadcast/multicast DNS..."

    if (-not (Ensure-TestAdapter)) { exit 1 }

    New-NetIPAddress -InterfaceAlias $AdapterAlias -IPAddress "192.168.200.1" -PrefixLength 24 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ServerAddresses "255.255.255.255","239.255.255.250","240.0.0.1"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-IpAddress $AdapterAlias "192.168.200.1") -and
          (Verify-DnsServers $AdapterAlias @("255.255.255.255", "239.255.255.250", "240.0.0.1"))
    if (-not $ok) { exit 1 }
}

function Setup-NoDns {
    Write-Host "Setting up test adapter with IP but NO DNS..."

    if (-not (Ensure-TestAdapter)) { exit 1 }

    New-NetIPAddress -InterfaceAlias $AdapterAlias -IPAddress "192.168.200.1" -PrefixLength 24 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ResetServerAddresses

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-IpAddress $AdapterAlias "192.168.200.1") -and
          (Verify-DnsServers $AdapterAlias @())
    if (-not $ok) { exit 1 }
}

function Setup-MultiDns {
    Write-Host "Setting up test adapter with multiple DNS servers..."

    if (-not (Ensure-TestAdapter)) { exit 1 }

    New-NetIPAddress -InterfaceAlias $AdapterAlias -IPAddress "192.168.200.1" -PrefixLength 24 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ServerAddresses "10.255.255.1","10.255.255.2"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-IpAddress $AdapterAlias "192.168.200.1") -and
          (Verify-DnsServers $AdapterAlias @("10.255.255.1", "10.255.255.2"))
    if (-not $ok) { exit 1 }
}

function Setup-FlappingSetup {
    Write-Host "Setting up test adapter for flapping test..."

    if (-not (Ensure-TestAdapter)) { exit 1 }

    New-NetIPAddress -InterfaceAlias $AdapterAlias -IPAddress "192.168.200.1" -PrefixLength 24 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ServerAddresses "10.255.255.1"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-DnsServers $AdapterAlias @("10.255.255.1"))
    if (-not $ok) { exit 1 }
}

function Setup-Ipv6 {
    Write-Host "Setting up test adapter with IPv6 DNS..."

    if (-not (Ensure-TestAdapter)) { exit 1 }

    # Ensure IPv4 config exists (adapter may have been reconfigured)
    New-NetIPAddress -InterfaceAlias $AdapterAlias -IPAddress "192.168.200.1" -PrefixLength 24 -ErrorAction SilentlyContinue

    # Add IPv6 unicast address (ULA range - passes is_valid_ipv6)
    New-NetIPAddress -InterfaceAlias $AdapterAlias -IPAddress "fd00::1" -PrefixLength 64 -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1

    # Set DNS with valid IPv6 (fd00::53) and link-local (fe80::dead:beef)
    # The link-local address should be filtered by is_valid_ipv6
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ServerAddresses "fd00::53","fe80::dead:beef","10.255.255.1"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-IpAddress $AdapterAlias "192.168.200.1")
    if (-not $ok) { exit 1 }

    # Verify IPv6 address
    $ipv6 = Get-NetIPAddress -InterfaceAlias $AdapterAlias -AddressFamily IPv6 -ErrorAction SilentlyContinue |
        Where-Object { $_.IPAddress -eq "fd00::1" }
    if (-not $ipv6) {
        Write-Host "  [VERIFY FAIL] IPv6 address fd00::1 not assigned" -ForegroundColor Red
        exit 1
    }
    Write-Host "  [VERIFY OK] IPv6 address: fd00::1" -ForegroundColor Green

    # Verify DNS includes our IPv6 entries
    $dns = Get-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ErrorAction SilentlyContinue
    $allDns = @()
    if ($dns) { foreach ($d in $dns) { $allDns += $d.ServerAddresses } }
    Write-Host "  [INFO] All DNS entries: $($allDns -join ', ')" -ForegroundColor Cyan
}

function Setup-Dedup {
    Write-Host "Setting up two adapters with duplicate DNS..."

    # First adapter
    if (-not (Ensure-TestAdapter)) { exit 1 }
    New-NetIPAddress -InterfaceAlias $AdapterAlias -IPAddress "192.168.200.1" -PrefixLength 24 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ServerAddresses "10.255.255.1"

    # Second adapter via second Hyper-V internal switch
    $Switch2 = "PNTestSwitch2"
    $Adapter2Alias = "vEthernet ($Switch2)"

    $existing = Get-VMSwitch -Name $Switch2 -ErrorAction SilentlyContinue
    if (-not $existing) {
        Write-Host "  Creating second Hyper-V switch '$Switch2'..."
        New-VMSwitch -Name $Switch2 -SwitchType Internal | Out-Null
        Start-Sleep -Seconds 3
    } else {
        Write-Host "  Second switch '$Switch2' already exists"
    }

    $Adapter2Object = Get-NetAdapter -Name $Adapter2Alias -ErrorAction SilentlyContinue
    if (-not $Adapter2Object) {
        Write-Host "  [ERROR] Adapter '$Adapter2Alias' did not appear" -ForegroundColor Red
        Get-NetAdapter | Format-Table Name, InterfaceDescription, Status -AutoSize
        exit 1
    }

    if ($Adapter2Object.Status -ne "Up") {
        Enable-NetAdapter -Name $Adapter2Alias -Confirm:$false
        Start-Sleep -Seconds 2
    }

    Remove-NetIPAddress -InterfaceAlias $Adapter2Alias -Confirm:$false -ErrorAction SilentlyContinue
    New-NetIPAddress -InterfaceAlias $Adapter2Alias -IPAddress "192.168.201.1" -PrefixLength 24 -ErrorAction SilentlyContinue
    # Same DNS as first adapter — must be deduplicated
    Set-DnsClientServerAddress -InterfaceAlias $Adapter2Alias -ServerAddresses "10.255.255.1"

    # Verify both adapters
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-DnsServers $AdapterAlias @("10.255.255.1")) -and
          (Verify-AdapterStatus $Adapter2Alias "Up") -and
          (Verify-IpAddress $Adapter2Alias "192.168.201.1") -and
          (Verify-DnsServers $Adapter2Alias @("10.255.255.1"))
    if (-not $ok) { exit 1 }

    Write-Host "  Both adapters configured with DNS 10.255.255.1" -ForegroundColor Green
}

function Setup-StaleVpn {
    Write-Host "Setting up test adapter simulating stale VPN DNS..."

    if (-not (Ensure-TestAdapter)) { exit 1 }

    New-NetIPAddress -InterfaceAlias $AdapterAlias -IPAddress "192.168.200.1" -PrefixLength 24 -ErrorAction SilentlyContinue
    # 10.255.255.1 has no actual DNS server listening — simulates stale VPN
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ServerAddresses "10.255.255.1"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-IpAddress $AdapterAlias "192.168.200.1") -and
          (Verify-DnsServers $AdapterAlias @("10.255.255.1"))
    if (-not $ok) { exit 1 }

    Write-Host "  Stale VPN scenario ready (10.255.255.1 is unreachable)" -ForegroundColor Green
}

function Setup-ApipaUnicast {
    Write-Host "Setting up adapter with APIPA unicast + DNS..."

    if (-not (Ensure-TestAdapter)) { exit 1 }

    # APIPA source address should make adapter unsuitable for DNS extraction.
    New-NetIPAddress -InterfaceAlias $AdapterAlias -IPAddress "169.254.200.1" -PrefixLength 16 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ServerAddresses "10.255.255.1"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-IpAddress $AdapterAlias "169.254.200.1") -and
          (Verify-DnsServers $AdapterAlias @("10.255.255.1"))
    if (-not $ok) { exit 1 }

    Write-Host "  APIPA scenario ready (adapter should be filtered by unicast validation)" -ForegroundColor Green
}

function Setup-DnsNoIp {
    Write-Host "Setting up adapter with DNS but no IPv4 address..."

    if (-not (Ensure-TestAdapter)) { exit 1 }

    # Do NOT assign IPv4 address; keep DNS configured to verify adapter is excluded.
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ServerAddresses "10.255.255.1"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-NoValidIpv4Address $AdapterAlias) -and
          (Verify-DnsServers $AdapterAlias @("10.255.255.1"))
    if (-not $ok) { exit 1 }

    Write-Host "  DNS-without-IP scenario ready (adapter should be filtered)" -ForegroundColor Green
}

function Teardown {
    Write-Host "Cleaning up network test state..."

    foreach ($name in @($SwitchName, "PNTestSwitch2")) {
        $switch = Get-VMSwitch -Name $name -ErrorAction SilentlyContinue
        if ($switch) {
            Remove-VMSwitch -Name $name -Force
            Write-Host "  Removed Hyper-V switch '$name'"
        }
    }

    Write-Host "  Cleanup complete"
}

# Dispatch
switch ($Scenario) {
    "loopback"        { Setup-Loopback }
    "disable"         { Setup-Disable }
    "metric"          { Setup-Metric }
    "broadcast"       { Setup-Broadcast }
    "no_dns"          { Setup-NoDns }
    "multi_dns"       { Setup-MultiDns }
    "flapping_setup"  { Setup-FlappingSetup }
    "ipv6"            { Setup-Ipv6 }
    "dedup"           { Setup-Dedup }
    "stale_vpn"       { Setup-StaleVpn }
    "apipa_unicast"   { Setup-ApipaUnicast }
    "dns_no_ip"       { Setup-DnsNoIp }
    "teardown"        { Teardown }
}
