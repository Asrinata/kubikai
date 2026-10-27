# Kübit FPS servisi: PresentMon (Intel, açık kaynak) bütün uygulamaların kare sunumlarını izler; her saniye en çok kare çizen
# uygulamanın FPS'ini C:\ProgramData\KubitPC\fps.json'a yazar. Oyun çalışmıyorsa dosya güncellenmez (ajan 3 sn'den eskiyi yok sayar).
# SYSTEM olarak çalışır (PresentMon ETW için yönetici ister); kurulum: FPS_KUR.bat.
$H = 'C:\ProgramData\KubitPC'
$atla = @('dwm.exe', 'explorer.exe', 'SearchHost.exe', 'StartMenuExperienceHost.exe', 'TextInputHost.exe', 'ShellExperienceHost.exe', '<unknown>')
while ($true) {
  $p = New-Object System.Diagnostics.Process
  $p.StartInfo.FileName = "$H\PresentMon.exe"
  $p.StartInfo.Arguments = '--output_stdout --no_console_stats --stop_existing_session --session_name KubitFPS'
  $p.StartInfo.UseShellExecute = $false; $p.StartInfo.RedirectStandardOutput = $true; $p.StartInfo.CreateNoWindow = $true
  [void]$p.Start()
  $baslik = $p.StandardOutput.ReadLine() -split ','
  $iUyg = [array]::IndexOf($baslik, 'Application')
  $say = @{}; $saniye = [DateTime]::UtcNow
  while (-not $p.HasExited) {
    $satir = $p.StandardOutput.ReadLine(); if ($satir -eq $null) { break }
    $uyg = ($satir -split ',')[$iUyg]
    if ($atla -notcontains $uyg) { $say[$uyg] = 1 + [int]$say[$uyg] }
    $simdi = [DateTime]::UtcNow; $gecen = ($simdi - $saniye).TotalSeconds
    if ($gecen -ge 1) {
      if ($say.Count) {
        $en = $say.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 1
        $fps = [math]::Round($en.Value / $gecen)
        if ($fps -ge 5) { @{ fps = $fps; uygulama = ($en.Key -replace '\.exe$', ''); zaman = [int][double]::Parse((Get-Date -UFormat %s)) } | ConvertTo-Json -Compress | Set-Content "$H\fps.json" -Encoding UTF8 }
      }
      $say = @{}; $saniye = $simdi
    }
  }
  Start-Sleep 5                                        # PresentMon kapandıysa yeniden başlat
}
