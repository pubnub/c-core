# setup_network_scenarios.ps1
# Creates mock network adapters and configurations for DNS discovery testing.
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
#  Adapter install / helpers
# ──────────────────────────────────────────────

function Install-LoopbackAdapter {
    Write-Host "  Installing Microsoft Loopback Adapter..."

    # Check if already installed
    $existing = Get-NetAdapter -Name "Loopback" -ErrorAction SilentlyContinue
    if ($existing) {
        Write-Host "  Loopback adapter already exists (status: $($existing.Status))"
        return $true
    }

    # Use devcon if available, otherwise pnputil
    $devcon = Get-Command devcon.exe -ErrorAction SilentlyContinue
    if ($devcon) {
        & devcon.exe install "$env:windir\INF\netloop.inf" "*MSLOOP" 2>&1
    } else {
        # Use pnputil + PowerShell approach
        try {
            $null = New-NetAdapter -Name "Loopback" -InterfaceDescription "Microsoft KM-TEST Loopback Adapter" -ErrorAction Stop
        } catch {
            Write-Host "  Trying alternative: installing via pnputil..."
            pnputil /add-driver "$env:windir\INF\netloop.inf" /install 2>&1

            $retries = 0
            do {
                Start-Sleep -Seconds 2
                $retries++
                $adapter = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*Loopback*" -or $_.InterfaceDescription -like "*KM-TEST*" }
            } while (-not $adapter -and $retries -lt 5)

            if ($adapter) {
                Rename-NetAdapter -Name $adapter.Name -NewName "Loopback" -ErrorAction SilentlyContinue
            }
        }
    }

    # Verify
    Start-Sleep -Seconds 2
    $adapter = Get-NetAdapter -Name "Loopback" -ErrorAction SilentlyContinue
    if (-not $adapter) {
        $adapter = Get-NetAdapter | Where-Object { $_.InterfaceDescription -like "*Loopback*" -or $_.InterfaceDescription -like "*KM-TEST*" }
        if ($adapter) {
            Rename-NetAdapter -Name $adapter.Name -NewName "Loopback"
        } else {
            Write-Warning "  Could not install Loopback adapter"
            return $false
        }
    }

    Write-Host "  Loopback adapter installed: $($adapter.Name) ($($adapter.Status))"
    return $true
}

function Ensure-LoopbackUp {
    $adapter = Get-NetAdapter -Name "Loopback" -ErrorAction SilentlyContinue
    if (-not $adapter) {
        Write-Warning "Loopback adapter not found"
        return $false
    }

    if ($adapter.Status -ne "Up") {
        Enable-NetAdapter -Name "Loopback" -Confirm:$false
        Start-Sleep -Seconds 2
    }

    # Clean existing config
    Remove-NetIPAddress -InterfaceAlias "Loopback" -Confirm:$false -ErrorAction SilentlyContinue
    Remove-NetRoute -InterfaceAlias "Loopback" -Confirm:$false -ErrorAction SilentlyContinue
    return $true
}


# ──────────────────────────────────────────────
#  Scenario setup + verification
# ──────────────────────────────────────────────

function Setup-Loopback {
    Write-Host "Setting up loopback adapter with fake DNS..."

    $installed = Install-LoopbackAdapter
    if (-not $installed) {
        Write-Warning "Skipping loopback setup - adapter not available"
        exit 0
    }

    if (-not (Ensure-LoopbackUp)) { exit 0 }

    New-NetIPAddress -InterfaceAlias "Loopback" -IPAddress "169.254.1.1" -PrefixLength 16 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias "Loopback" -ServerAddresses "10.255.255.1"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus "Loopback" "Up") -and
          (Verify-IpAddress "Loopback" "169.254.1.1") -and
          (Verify-DnsServers "Loopback" @("10.255.255.1"))
    if (-not $ok) { exit 1 }
}

function Setup-Disable {
    Write-Host "Disabling loopback adapter..."

    $adapter = Get-NetAdapter -Name "Loopback" -ErrorAction SilentlyContinue
    if (-not $adapter) {
        Write-Warning "Loopback adapter not found - skipping"
        exit 0
    }

    Disable-NetAdapter -Name "Loopback" -Confirm:$false
    Start-Sleep -Seconds 2

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = Verify-AdapterStatus "Loopback" "Disabled"
    if (-not $ok) { exit 1 }
}

function Setup-Metric {
    Write-Host "Setting high metric on loopback adapter..."

    $adapter = Get-NetAdapter -Name "Loopback" -ErrorAction SilentlyContinue
    if (-not $adapter) {
        Write-Warning "Loopback adapter not found - skipping"
        exit 0
    }

    if ($adapter.Status -ne "Up") {
        Enable-NetAdapter -Name "Loopback" -Confirm:$false
        Start-Sleep -Seconds 2
    }

    Set-NetIPInterface -InterfaceAlias "Loopback" -InterfaceMetric 9999 -ErrorAction SilentlyContinue

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus "Loopback" "Up") -and
          (Verify-Metric "Loopback" 9999)
    if (-not $ok) { exit 1 }
}

function Setup-Broadcast {
    Write-Host "Setting up loopback adapter with broadcast/multicast DNS..."

    $installed = Install-LoopbackAdapter
    if (-not $installed) {
        Write-Warning "Skipping broadcast setup - adapter not available"
        exit 0
    }

    if (-not (Ensure-LoopbackUp)) { exit 0 }

    New-NetIPAddress -InterfaceAlias "Loopback" -IPAddress "169.254.1.1" -PrefixLength 16 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias "Loopback" -ServerAddresses "255.255.255.255","239.255.255.250"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus "Loopback" "Up") -and
          (Verify-IpAddress "Loopback" "169.254.1.1") -and
          (Verify-DnsServers "Loopback" @("255.255.255.255", "239.255.255.250"))
    if (-not $ok) { exit 1 }
}

function Setup-NoDns {
    Write-Host "Setting up loopback adapter with IP but NO DNS..."

    $installed = Install-LoopbackAdapter
    if (-not $installed) {
        Write-Warning "Skipping no-DNS setup - adapter not available"
        exit 0
    }

    if (-not (Ensure-LoopbackUp)) { exit 0 }

    New-NetIPAddress -InterfaceAlias "Loopback" -IPAddress "169.254.1.1" -PrefixLength 16 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias "Loopback" -ResetServerAddresses

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus "Loopback" "Up") -and
          (Verify-IpAddress "Loopback" "169.254.1.1") -and
          (Verify-DnsServers "Loopback" @())
    if (-not $ok) { exit 1 }
}

function Setup-MultiDns {
    Write-Host "Setting up loopback adapter with multiple DNS servers..."

    $installed = Install-LoopbackAdapter
    if (-not $installed) {
        Write-Warning "Skipping multi-DNS setup - adapter not available"
        exit 0
    }

    if (-not (Ensure-LoopbackUp)) { exit 0 }

    New-NetIPAddress -InterfaceAlias "Loopback" -IPAddress "169.254.1.1" -PrefixLength 16 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias "Loopback" -ServerAddresses "10.255.255.1","10.255.255.2"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus "Loopback" "Up") -and
          (Verify-IpAddress "Loopback" "169.254.1.1") -and
          (Verify-DnsServers "Loopback" @("10.255.255.1", "10.255.255.2"))
    if (-not $ok) { exit 1 }
}

function Setup-FlappingSetup {
    Write-Host "Setting up loopback adapter for flapping test..."

    $installed = Install-LoopbackAdapter
    if (-not $installed) {
        Write-Warning "Skipping flapping setup - adapter not available"
        exit 0
    }

    if (-not (Ensure-LoopbackUp)) { exit 0 }

    New-NetIPAddress -InterfaceAlias "Loopback" -IPAddress "169.254.1.1" -PrefixLength 16 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias "Loopback" -ServerAddresses "10.255.255.1"

    # Verify
    Write-Host "  Verifying state..." -ForegroundColor Yellow
    $ok = (Verify-AdapterStatus "Loopback" "Up") -and
          (Verify-DnsServers "Loopback" @("10.255.255.1"))
    if (-not $ok) { exit 1 }
}

function Teardown {
    Write-Host "Cleaning up network test state..."

    $adapter = Get-NetAdapter -Name "Loopback" -ErrorAction SilentlyContinue
    if ($adapter) {
        Remove-NetIPAddress -InterfaceAlias "Loopback" -Confirm:$false -ErrorAction SilentlyContinue
        Remove-NetRoute -InterfaceAlias "Loopback" -Confirm:$false -ErrorAction SilentlyContinue
        Disable-NetAdapter -Name "Loopback" -Confirm:$false -ErrorAction SilentlyContinue
        Write-Host "  Loopback adapter disabled and cleaned up"
    } else {
        Write-Host "  No Loopback adapter found - nothing to clean"
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
