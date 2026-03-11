# run_tests.ps1
# Orchestrates DNS discovery test phases.
#
# Usage: .\run_tests.ps1 [-TestExe <path>]
# Exit code: total number of test failures (0 = all pass)

param(
    [string]$TestExe = ".\dns_discovery_test.exe",
    [string]$ValidatedTestExe = ".\dns_discovery_test_validated.exe",
    [switch]$FailOnSetupError
)

$ErrorActionPreference = "Continue"
$totalFailures = 0

function Run-Phase {
    param(
        [string]$Phase,
        [string]$Description,
        [scriptblock]$Setup = $null,
        [string]$Scenario,
        [string]$TestExeOverride = ""
    )

    Write-Host ""
    Write-Host "============================================" -ForegroundColor Cyan
    Write-Host " Phase $Phase`: $Description" -ForegroundColor Cyan
    Write-Host "============================================" -ForegroundColor Cyan

    # Run setup if provided
    if ($Setup) {
        Write-Host "Running setup..." -ForegroundColor Yellow
        try {
            & $Setup | Out-Host
            if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
                Write-Host "  Setup exited with code $LASTEXITCODE" -ForegroundColor Red
                if ($FailOnSetupError) {
                    Write-Host "  Treating setup failure as phase failure." -ForegroundColor Red
                    return 1
                }
                Write-Host "  Skipping phase." -ForegroundColor Yellow
                return 0
            }
        } catch {
            Write-Host "  Setup failed: $_" -ForegroundColor Red
            if ($FailOnSetupError) {
                Write-Host "  Treating setup failure as phase failure." -ForegroundColor Red
                return 1
            }
            Write-Host "  Skipping phase." -ForegroundColor Yellow
            return 0
        }
    }

    # Run test
    $exe = if ($TestExeOverride) { $TestExeOverride } else { $TestExe }
    Write-Host "Running tests ($exe)..." -ForegroundColor Yellow
    & $exe $Scenario | Out-Host
    $exitCode = $LASTEXITCODE

    if ($exitCode -eq 0) {
        Write-Host "Phase passed." -ForegroundColor Green
    } else {
        Write-Host "Phase had $exitCode failure(s)." -ForegroundColor Red
    }

    return $exitCode
}

# Verify test executable exists
if (-not (Test-Path $TestExe)) {
    Write-Host "ERROR: Test executable not found: $TestExe" -ForegroundColor Red
    exit 1
}

Write-Host "DNS Discovery Test Suite" -ForegroundColor White
Write-Host "========================" -ForegroundColor White
Write-Host "Test executable: $TestExe"
Write-Host "Hostname: $env:COMPUTERNAME"
Write-Host "Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"

# Phase 1: Baseline (no network manipulation)
$totalFailures += (Run-Phase `
    -Phase "1" `
    -Description "Baseline - standard discovery" `
    -Scenario "baseline")

# Phase 2: Loopback adapter with fake DNS
$totalFailures += (Run-Phase `
    -Phase "2" `
    -Description "Loopback adapter filtering" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario loopback } `
    -Scenario "loopback")

# Phase 3: Disabled adapter
$totalFailures += (Run-Phase `
    -Phase "3" `
    -Description "Disabled adapter filtering" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario disable } `
    -Scenario "disabled")

# Phase 5: Metric ordering
$totalFailures += (Run-Phase `
    -Phase "5" `
    -Description "Metric-based ordering" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario metric } `
    -Scenario "metric")

# Phase 6: IPv6 (with injected IPv6 DNS on test adapter)
$totalFailures += (Run-Phase `
    -Phase "6" `
    -Description "IPv6 discovery" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario ipv6 } `
    -Scenario "ipv6")

# Phase 7: Concurrency
$totalFailures += (Run-Phase `
    -Phase "7" `
    -Description "Concurrency stress test" `
    -Scenario "concurrent")

# Phase 8: Buffer edge cases (no network manipulation)
$totalFailures += (Run-Phase `
    -Phase "8" `
    -Description "Buffer edge cases" `
    -Scenario "buffer")

# Phase 9: Result stability (no network manipulation)
$totalFailures += (Run-Phase `
    -Phase "9" `
    -Description "Result stability (50 iterations)" `
    -Scenario "stability")

# Phase 10: Broadcast/multicast DNS filtering
$totalFailures += (Run-Phase `
    -Phase "10" `
    -Description "Broadcast/multicast DNS filtering" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario broadcast } `
    -Scenario "broadcast")

# Phase 11: No-DNS adapter
$totalFailures += (Run-Phase `
    -Phase "11" `
    -Description "No-DNS adapter handling" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario no_dns } `
    -Scenario "no_dns")

# Phase 12: Adapter flapping
$totalFailures += (Run-Phase `
    -Phase "12" `
    -Description "Adapter flapping stress test" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario flapping_setup } `
    -Scenario "flapping")

# Phase 13: Multiple DNS per adapter
$totalFailures += (Run-Phase `
    -Phase "13" `
    -Description "Multiple DNS servers per adapter" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario multi_dns } `
    -Scenario "multi_dns")

# Phase 14: Boundary input tests (no network manipulation)
$totalFailures += (Run-Phase `
    -Phase "14" `
    -Description "Boundary input tests" `
    -Scenario "boundary")

# Phase 15: Cross-adapter deduplication
$totalFailures += (Run-Phase `
    -Phase "15" `
    -Description "Cross-adapter deduplication" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario dedup } `
    -Scenario "dedup")

# Phase 16: APIPA unicast adapter filtering
$totalFailures += (Run-Phase `
    -Phase "16" `
    -Description "APIPA unicast adapter filtering" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario apipa_unicast } `
    -Scenario "apipa_unicast")

# Phase 17: DNS configured but no IPv4 unicast on adapter
$totalFailures += (Run-Phase `
    -Phase "17" `
    -Description "DNS-without-unicast adapter filtering" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario dns_no_ip } `
    -Scenario "dns_no_ip")

# Phase 18: Stale VPN control case on non-validated build
$totalFailures += (Run-Phase `
    -Phase "18" `
    -Description "Stale VPN DNS visible (no validation build)" `
    -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario stale_vpn } `
    -Scenario "stale_vpn_no_validation")

# Phase 19: Stale VPN DNS validation (uses validated exe with timeout)
if (Test-Path $ValidatedTestExe) {
    $totalFailures += (Run-Phase `
        -Phase "19" `
        -Description "Stale VPN DNS validation" `
        -Setup { & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario stale_vpn } `
        -Scenario "stale_vpn" `
        -TestExeOverride $ValidatedTestExe)
} else {
    Write-Host ""
    Write-Host "============================================" -ForegroundColor Cyan
    Write-Host " Phase 19: Stale VPN DNS validation" -ForegroundColor Cyan
    Write-Host "============================================" -ForegroundColor Cyan
    Write-Host "  Skipped (validated exe not found: $ValidatedTestExe)" -ForegroundColor Yellow
}

# Teardown
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Teardown" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
try {
    & powershell -ExecutionPolicy Bypass -File "$PSScriptRoot\setup_network_scenarios.ps1" -Scenario teardown | Out-Host
} catch {
    Write-Host "  Teardown warning: $_" -ForegroundColor Yellow
}

# Summary
Write-Host ""
Write-Host "============================================" -ForegroundColor White
Write-Host " FINAL SUMMARY" -ForegroundColor White
Write-Host "============================================" -ForegroundColor White

if ($totalFailures -eq 0) {
    Write-Host "All phases passed." -ForegroundColor Green
} else {
    Write-Host "Total failures: $totalFailures" -ForegroundColor Red
}

exit $totalFailures
