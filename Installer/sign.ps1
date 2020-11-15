$MyInvocation.MyCommand.Path | Split-Path | Push-Location
$cert = Get-PfxData -Password (  ConvertTo-SecureString $Env:JUMPCUT_KEY_PASSWORD -AsPlainText -Force )  -FilePath ..\JumpcutUWP\JumpcutUWP_Key.pfx;
$thumbprint = $cert.EndEntityCertificates.Thumbprint;
echo $thumbprint;
&"C:\Program Files (x86)\Windows Kits\10\App Certification Kit\signtool.exe" sign /f ..\JumpcutUWP\JumpcutUWP_Key.pfx /d "Clipboard History Tool" /p $Env:JUMPCUT_KEY_PASSWORD /v /sha1 $thumbprint /t "http://timestamp.comodoca.com/authenticode" Release/Installer.msi
