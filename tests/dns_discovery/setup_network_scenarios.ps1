# setup_network_scenarios.ps1
# Creates mock network adapters and configurations for DNS discovery testing.
#
# Usage: .\setup_network_scenarios.ps1 -Scenario <name>
# Requires: Administrator privileges

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("loopback", "disable", "metric", "broadcast", "no_dns", "multi_dns", "flapping_setup", "teardown")]
    [string]$Scenario
)

$ErrorActionPreference = "Stop"

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
            # Add loopback adapter via WMI/CIM
            $null = New-NetAdapter -Name "Loopback" -InterfaceDescription "Microsoft KM-TEST Loopback Adapter" -ErrorAction Stop
        } catch {
            Write-Host "  Trying alternative: installing via pnputil..."
            pnputil /add-driver "$env:windir\INF\netloop.inf" /install 2>&1

            # Wait for adapter to appear
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
        # Try to find by description
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

function Setup-Loopback {
    Write-Host "Setting up loopback adapter with fake DNS..."

    $installed = Install-LoopbackAdapter
    if (-not $installed) {
        Write-Warning "Skipping loopback setup - adapter not available"
        exit 0
    }

    if (-not (Ensure-LoopbackUp)) { exit 0 }

    # Assign APIPA-range IP and unreachable DNS
    New-NetIPAddress -InterfaceAlias "Loopback" -IPAddress "169.254.1.1" -PrefixLength 16 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias "Loopback" -ServerAddresses "10.255.255.1"

    Write-Host "  Loopback adapter configured:"
    Write-Host "    IP: 169.254.1.1/16 (APIPA)"
    Write-Host "    DNS: 10.255.255.1 (unreachable)"
    Get-NetAdapter -Name "Loopback" | Format-Table Name, Status, InterfaceIndex -AutoSize
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

    $status = (Get-NetAdapter -Name "Loopback").Status
    Write-Host "  Loopback adapter status: $status"
}

function Setup-Metric {
    Write-Host "Setting high metric on loopback adapter..."

    $adapter = Get-NetAdapter -Name "Loopback" -ErrorAction SilentlyContinue
    if (-not $adapter) {
        Write-Warning "Loopback adapter not found - skipping"
        exit 0
    }

    # Re-enable if disabled
    if ($adapter.Status -ne "Up") {
        Enable-NetAdapter -Name "Loopback" -Confirm:$false
        Start-Sleep -Seconds 2
    }

    # Set very high metric so it sorts last
    Set-NetIPInterface -InterfaceAlias "Loopback" -InterfaceMetric 9999 -ErrorAction SilentlyContinue
    Write-Host "  Loopback adapter metric set to 9999"
}

function Setup-Broadcast {
    Write-Host "Setting up loopback adapter with broadcast/multicast DNS..."

    $installed = Install-LoopbackAdapter
    if (-not $installed) {
        Write-Warning "Skipping broadcast setup - adapter not available"
        exit 0
    }

    if (-not (Ensure-LoopbackUp)) { exit 0 }

    # Assign APIPA-range IP and broadcast + multicast DNS
    New-NetIPAddress -InterfaceAlias "Loopback" -IPAddress "169.254.1.1" -PrefixLength 16 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias "Loopback" -ServerAddresses "255.255.255.255","239.255.255.250"

    Write-Host "  Loopback adapter configured:"
    Write-Host "    IP: 169.254.1.1/16 (APIPA)"
    Write-Host "    DNS: 255.255.255.255 (broadcast), 239.255.255.250 (multicast)"
    Get-NetAdapter -Name "Loopback" | Format-Table Name, Status, InterfaceIndex -AutoSize
}

function Setup-NoDns {
    Write-Host "Setting up loopback adapter with IP but NO DNS..."

    $installed = Install-LoopbackAdapter
    if (-not $installed) {
        Write-Warning "Skipping no-DNS setup - adapter not available"
        exit 0
    }

    if (-not (Ensure-LoopbackUp)) { exit 0 }

    # Assign IP but explicitly clear DNS
    New-NetIPAddress -InterfaceAlias "Loopback" -IPAddress "169.254.1.1" -PrefixLength 16 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias "Loopback" -ResetServerAddresses

    Write-Host "  Loopback adapter configured:"
    Write-Host "    IP: 169.254.1.1/16 (APIPA)"
    Write-Host "    DNS: (none)"
    Get-NetAdapter -Name "Loopback" | Format-Table Name, Status, InterfaceIndex -AutoSize
}

function Setup-MultiDns {
    Write-Host "Setting up loopback adapter with multiple DNS servers..."

    $installed = Install-LoopbackAdapter
    if (-not $installed) {
        Write-Warning "Skipping multi-DNS setup - adapter not available"
        exit 0
    }

    if (-not (Ensure-LoopbackUp)) { exit 0 }

    # Assign APIPA-range IP and TWO unreachable DNS servers
    New-NetIPAddress -InterfaceAlias "Loopback" -IPAddress "169.254.1.1" -PrefixLength 16 -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceAlias "Loopback" -ServerAddresses "10.255.255.1","10.255.255.2"

    Write-Host "  Loopback adapter configured:"
    Write-Host "    IP: 169.254.1.1/16 (APIPA)"
    Write-Host "    DNS: 10.255.255.1, 10.255.255.2"
    Get-NetAdapter -Name "Loopback" | Format-Table Name, Status, InterfaceIndex -AutoSize
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

    Write-Host "  Loopback adapter ready for flapping test"
}

function Teardown {
    Write-Host "Cleaning up network test state..."

    $adapter = Get-NetAdapter -Name "Loopback" -ErrorAction SilentlyContinue
    if ($adapter) {
        # Remove IP addresses
        Remove-NetIPAddress -InterfaceAlias "Loopback" -Confirm:$false -ErrorAction SilentlyContinue
        Remove-NetRoute -InterfaceAlias "Loopback" -Confirm:$false -ErrorAction SilentlyContinue

        # Disable the adapter
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
