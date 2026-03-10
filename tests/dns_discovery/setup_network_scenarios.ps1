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
    [ValidateSet("loopback", "disable", "metric", "broadcast", "no_dns", "multi_dns", "flapping_setup", "teardown")]
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
    Set-DnsClientServerAddress -InterfaceAlias $AdapterAlias -ServerAddresses "255.255.255.255","239.255.255.250"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus $AdapterAlias "Up") -and
          (Verify-IpAddress $AdapterAlias "192.168.200.1") -and
          (Verify-DnsServers $AdapterAlias @("255.255.255.255", "239.255.255.250"))
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

function Teardown {
    Write-Host "Cleaning up network test state..."

    $switch = Get-VMSwitch -Name $SwitchName -ErrorAction SilentlyContinue
    if ($switch) {
        Remove-VMSwitch -Name $SwitchName -Force
        Write-Host "  Removed Hyper-V switch '$SwitchName'"
    } else {
        Write-Host "  No test switch found - nothing to clean"
    }
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
    "teardown"        { Teardown }
}
