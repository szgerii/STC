$PROJECT_ROOT = (Resolve-Path "$PSScriptRoot\..").Path
$BASE_BUILD_DIR = Join-Path $PROJECT_ROOT "build"

Write-Host "project root: $PROJECT_ROOT" -ForegroundColor DarkCyan
Write-Host "build root:   $BASE_BUILD_DIR" -ForegroundColor DarkCyan

$clean_first = $args -contains "--clean-first";
$config_only = $args -contains "--config-only";
$reconfig = $args -contains "--reconfig";
$interactive = ($args -contains "-i") -or ($args -contains "--interactive");

# assumes msvc is the default c/cpp compiler
# if this is not the case and cl is in path, it can simply be added here
# otherwise, msvc configs can also just be removed
$configs = @(
    @{ Name = "gcc-dbg";     Gen = "Ninja"; CC = "gcc";   CXX = "g++";     Type = "Debug";          Tidy = $false; Profile = $false },
    @{ Name = "gcc-rel";     Gen = "Ninja"; CC = "gcc";   CXX = "g++";     Type = "Release";        Tidy = $false; Profile = $false },
    @{ Name = "msvc-dbg";    Gen = "";      CC = "";      CXX = "";        Type = "Debug";          Tidy = $false; Profile = $false },
    @{ Name = "msvc-rel";    Gen = "";      CC = "";      CXX = "";        Type = "Release";        Tidy = $false; Profile = $false },
    @{ Name = "clang-dbg";   Gen = "Ninja"; CC = "clang"; CXX = "clang++"; Type = "Debug";          Tidy = $false; Profile = $false },
    @{ Name = "clang-rel";   Gen = "Ninja"; CC = "clang"; CXX = "clang++"; Type = "Release";        Tidy = $false; Profile = $false },
    @{ Name = "tidy";        Gen = "Ninja"; CC = "clang"; CXX = "clang++"; Type = "Debug";          Tidy = $true;  Profile = $false },
    @{ Name = "gcc-rwdi";    Gen = "Ninja"; CC = "gcc";   CXX = "g++";     Type = "RelWithDebInfo"; Tidy = $false; Profile = $true }
)

$build_timer = [System.Diagnostics.Stopwatch]::StartNew()

$successful_builds = 0
$skipped_builds = 0
$current_step = 0

foreach ($cfg in $configs) {
    $current_step++
    $progress = "($current_step/$($configs.Count))"

    $do_build = -not $config_only
    $skip_cfg = $false

    if ($interactive) {
        $valid_input = $false

        while (-not $valid_input) {
            $valid_input = $true
            
            Write-Host "`n$progress Action for $($cfg.Name): config and [b]uild, [c]onfig only, [s]kip" -ForegroundColor Magenta
            $action = Read-Host "Action"
            
            switch ($action.ToLower().Trim()) {
                's' { 
                    Write-Host "`n$progress >> skipping $($cfg.Name)." -ForegroundColor DarkGray
                    $skip_cfg = $true
                }
                'c' { 
                    $do_build = $false 
                }
                'b' { 
                    $do_build = $true 
                }
                default { 
                    Write-Host "Invalid action provided (expected: b, c, s)" -ForegroundColor Yellow
                    $valid_input = $false
                }
            }
        }

        if ($skip_cfg) {
            $skipped_builds++;
            continue;
        }
    }

    $build_dir = Join-Path $BASE_BUILD_DIR $cfg.Name;
    $build_dir_exists = (Test-Path $build_dir);
    
    $do_reconfig = $reconfig

    if ($build_dir_exists -and $interactive -and (-not $reconfig)) {
        $valid_input = $false

        while (-not $valid_input) {
            $valid_input = $true
            
            Write-Host "`n$progress Reconfigure $($cfg.Name): [y]es, [n]o" -ForegroundColor Magenta
            $action = Read-Host "Action"
            
            switch ($action.ToLower().Trim()) {
                'y' { 
                    $do_reconfig = $true
                }
                'n' { 
                    $do_reconfig = $false 
                }
                default { 
                    Write-Host "Invalid action provided (expected: y, n)" -ForegroundColor Yellow
                    $valid_input = $false
                }
            }
        }
    }

    $do_config = (-Not $build_dir_exists) -or $do_reconfig
    if ($do_config) {
        if ($do_reconfig) {
            Write-Host "`n$progress >> deleting directory $build_dir and reconfiguring $($cfg.Name)..." -ForegroundColor Cyan
            Remove-Item -Recurse -Force $build_dir
        } else {
            Write-Host "`n$progress >> directory $build_dir not found, configuring $($cfg.Name)..." -ForegroundColor Cyan
        }

        $cmake_config_args = @("-S", $PROJECT_ROOT, "-B", $build_dir)

        if ($cfg.Gen) { $cmake_config_args += "-G", $cfg.Gen }
        if ($cfg.CC)  { $cmake_config_args += "-DCMAKE_C_COMPILER=$($cfg.CC)" }
        if ($cfg.CXX) { $cmake_config_args += "-DCMAKE_CXX_COMPILER=$($cfg.CXX)" }

        $cmake_config_args += "-DCMAKE_BUILD_TYPE=$($cfg.Type)"
        
        $cmake_config_args += "-DBUILD_TESTING=OFF"
        if ($cfg.Tidy)    { $cmake_config_args += "-DSTC_USE_TIDY=ON" }
        if ($cfg.Profile) { $cmake_config_args += "-DSTC_ENABLE_PROFILING=ON" }

        Write-Host "$progress >> cmake $($cmake_config_args -join ' ')`n" -ForegroundColor Yellow
        & cmake $cmake_config_args
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "$progress >> configuration failed for $($cfg.Name), skipping build." -ForegroundColor Red
            continue
        }
    }
    else {
        Write-Host "`n$progress >> directory $build_dir found, skipping config." -ForegroundColor DarkGray
    }

    if (-Not $do_build) {
        Write-Host "`n$progress >> only configuration requested for $($cfg.Name), skipping build." -ForegroundColor Green
        $successful_builds++;
        continue
    }

    Write-Host "`n$progress >> building $($cfg.Name)..." -ForegroundColor Cyan

    # append --config because msvc
    $cmake_build_args = @("--build", $build_dir, "--config", $($cfg.Type), "-j")

    $do_clean_first = $clean_first

    if ($interactive -and (-not $clean_first) -and (-not $do_config)) {
        $valid_input = $false

        while (-not $valid_input) {
            $valid_input = $true
            
            Write-Host "`n$progress Clean $($cfg.Name) before build: [y]es, [n]o" -ForegroundColor Magenta
            $action = Read-Host "Action"
            
            switch ($action.ToLower().Trim()) {
                'y' { 
                    $do_clean_first = $true
                }
                'n' { 
                    $do_clean_first = $false 
                }
                default { 
                    Write-Host "Invalid action provided (expected: y, n)" -ForegroundColor Yellow
                    $valid_input = $false
                }
            }
        }
    }

    if ($do_clean_first) {
        $cmake_build_args += "--clean-first";
    }

    Write-Host "$progress >> cmake $($cmake_build_args -join ' ')`n" -ForegroundColor Yellow
    & cmake $cmake_build_args

    if ($LASTEXITCODE -eq 0) {
        $successful_builds++;
        Write-Host "`n$progress >> build successful for $($cfg.Name)" -ForegroundColor Green
    } else {
        Write-Host "`n$progress >> build failed for $($cfg.Name)" -ForegroundColor Red
    }
}

$build_timer.Stop()
$elapsed_str = "{0:mm}m {0:ss}s" -f $build_timer.Elapsed
$attempted_builds = $configs.Count - $skipped_builds

$report_col = if ($successful_builds -eq $attempted_builds) { "Green" } else { "Yellow" }
if (($successful_builds -eq 0) -and ($attempted_builds -ne 0)) { $report_col = "Red" }

Write-Host "`n--- build report ---" -ForegroundColor Cyan
Write-Host "total time: $elapsed_str`n" -ForegroundColor Cyan
Write-Host "$successful_builds/$attempted_builds builds succeeded, $skipped_builds targets skipped`n" -ForegroundColor $report_col
