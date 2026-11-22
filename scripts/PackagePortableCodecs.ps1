# Portable codec packaging for LameXP Next (Windows x64).
# Gyan's release Essentials includes FFmpeg, FFprobe and the libmp3lame encoder.
# Use upstream's matching SHA256 sidecar; provenance/license review required before PUBLIC release.
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$base = 'https://www.gyan.dev/ffmpeg/builds'
$work = Join-Path $env:RUNNER_TEMP 'lamexp-codec-staging'
New-Item -ItemType Directory -Force $work | Out-Null
$archive = Join-Path $work 'ffmpeg-release-essentials.zip'
$hashfile = Join-Path $work 'ffmpeg-release-essentials.zip.sha256'
Invoke-WebRequest -Uri "$base/ffmpeg-release-essentials.zip" -OutFile $archive
Invoke-WebRequest -Uri "$base/ffmpeg-release-essentials.zip.sha256" -OutFile $hashfile
$expected = [regex]::Match((Get-Content $hashfile -Raw), '[a-fA-F0-9]{64}').Value.ToLowerInvariant()
if (!$expected) { throw 'Missing or invalid upstream SHA256 checksum' }
$actual = (Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne $expected) { throw "FFmpeg archive checksum mismatch: expected $expected, got $actual" }
$extract = Join-Path $work 'extracted'
Expand-Archive -Path $archive -DestinationPath $extract -Force
$ffmpeg = @(Get-ChildItem $extract -Filter 'ffmpeg.exe' -Recurse -File)
$ffprobe = @(Get-ChildItem $extract -Filter 'ffprobe.exe' -Recurse -File)
if ($ffmpeg.Count -ne 1 -or $ffprobe.Count -ne 1) { throw 'Archive must contain exactly one ffmpeg.exe and ffprobe.exe' }
$codecdir = Join-Path $PWD 'dist/codecs'
New-Item -ItemType Directory -Force $codecdir | Out-Null
Copy-Item $ffmpeg[0].FullName $codecdir
Copy-Item $ffprobe[0].FullName $codecdir
$licenseDir = Join-Path $PWD 'dist/licenses/ffmpeg'
New-Item -ItemType Directory -Force $licenseDir | Out-Null
# Copy original bundled docs and legal texts when present.
$legal = @(Get-ChildItem $extract -Recurse -File | Where-Object { $_.Name -match '^(LICENSE|COPYING|README)(\..*)?$' })
foreach ($f in $legal) { Copy-Item $f.FullName (Join-Path $licenseDir $f.Name) -Force }
@"
Downloaded from: $base/ffmpeg-release-essentials.zip
Archive SHA256: $actual
Upstream checksum: $base/ffmpeg-release-essentials.zip.sha256
FFmpeg source / licensing: https://ffmpeg.org/legal.html
Windows build provider: https://www.gyan.dev/ffmpeg/builds/
FFmpeg Essentials includes libmp3lame; no separate lame.exe bundled.
For PUBLIC binary redistribution: verify corresponding source availability, third-party licenses,
and the specific build configuration and include appropriate legal files/source offers.
"@ | Set-Content -Path (Join-Path $licenseDir 'BINARY-PROVENANCE.txt') -Encoding utf8
& (Join-Path $codecdir 'ffmpeg.exe') -hide_banner -version
if ($LASTEXITCODE -ne 0) { throw 'Bundled FFmpeg version check failed' }
& (Join-Path $codecdir 'ffprobe.exe') -hide_banner -version
if ($LASTEXITCODE -ne 0) { throw 'Bundled FFprobe version check failed' }
$encoders = & (Join-Path $codecdir 'ffmpeg.exe') -hide_banner -encoders 2>&1 | Out-String
if ($encoders -notmatch 'libmp3lame') { throw 'FFmpeg essentials lacks required libmp3lame encoder' }
# Functional test: synthesize input, encode MP3, inspect output.
$testMp3 = Join-Path $work 'smoke.mp3'
& (Join-Path $codecdir 'ffmpeg.exe') -hide_banner -loglevel error -f lavfi -i 'sine=frequency=440:duration=1' -c:a libmp3lame -q:a 4 -y $testMp3
if ($LASTEXITCODE -ne 0 -or !(Test-Path $testMp3)) { throw 'Bundled MP3 encoding test failed' }
$codec = & (Join-Path $codecdir 'ffprobe.exe') -v error -select_streams a:0 -show_entries stream=codec_name -of default=noprint_wrappers=1:nokey=1 $testMp3
if ($LASTEXITCODE -ne 0 -or ($codec | Out-String).Trim() -ne 'mp3') { throw 'Bundled MP3 verification failed' }
Write-Host "Portable codecs verified. Upstream archive SHA256: $actual"
