# PPLRunner

This project is to enable running 'arbitrary`*`' process as an Anti Malware Protected-Process-Light (PPL), for research purposes.

(`*` See the [Restrictions](#restrictions) section for more details)

# Overview
System protected process is a security model in Windows designed to protect system and anti-virus processes from
tampering or introspection, even by Administrators/SYSTEM.

Processes started as an Anti Malware 'Protected Process-Light' (PPL) are restricted in what they can do, can only load signed code, but cannot be debugged, inspected, or stopped by non-Protected Processes. Additionally, they can 
get access to special data, such as the `Microsoft-Windows-Threat-Intelligence` ETW Provider.

This project creates an Early-Launch Anti Malware (ELAM) driver and usermode service. The service will launch a configurable child process when it starts which will also be marked as PPL.

The child binary must be signed with the same certificate as the service, along with some other [restrictions](#restrictions), but can otherwise be any binary and commandline arguments you chose.

Honestly I'm not doing a good job of explaining what ELAM and PPL are, instead I recommend starting here:
- Chapter 3, Windows Internals Part 1 (#1 resource for anything Windows honestly)
- https://docs.microsoft.com/en-us/windows/win32/services/protecting-anti-malware-services-
- https://www.crowdstrike.com/blog/protected-processes-part-3-windows-pki-internals-signing-levels-scenarios-signers-root-keys/
- https://googleprojectzero.blogspot.com/2018/10/injecting-code-into-windows-protected.html


# Setup
Make sure you have Windows SDKs installed.

Open `generate_cert.ps1` and `sign_file.ps1`, and change the `$password` variable to something else (they must match each other).

Run `generate_cert.ps1`. This will generate a `ppl_runner.pfx` with a new private and public certificate.
This will be used to sign all binaries used by PPLRunner.

# Build
Build `ppl_runner.sln`. This will produce 3 binaries:
### elam_driver.sys
The ELAM Kernel Driver that has the certificate information in it.
The driver doesn't actually do anything, and won't actually be loaded, it is just used as a vessel for the
signing certificate.

### ppl_runner.exe
The Service installer and binary. As a PPL service, when started it will launch a child process,
also as PPL, then stop and exit.

### child_example.exe
An example executable that will be signed with the correct certificate by Visual Studio at build time.
PPLRunner can run almost any binary, this is just an example that will be automatically signed.

# Install
**NOTE** Only install on a testing machine, not production/your home PC.

1. Once built, copy `elam_driver.sys` and `ppl_runner.exe` to a folder on the target machine.

2. Enable test signing by running this from an elevated prompt, then reboot:
```bash
bcdedit /set testsigning on
```

3. From an elevated command prompt, browse to the folder containing the copied executables and run:
```bash
ppl_runner.exe install
```
This should install a service named `ppl_runner`.

# Configure
To sign a binary to run, sign it with the `ppl_runner.pfx` cert, using either the `sign_file.ps1` script,
or just running `signtool.exe` yourself. If you don't have `signtool.exe`, it is in the Windows SDKs.

Create the registry key `HKLM\SOFTWARE\PPL_RUNNER`.
Set the default/empty key to be a `REG_SZ`, containing the full path to the binary to execute,
and any commandline argument. e.g. from the commandline:
```cmd
REG.exe ADD HKLM\SOFTWARE\PPL_RUNNER /ve /t REG_SZ /d "C:\path\to\binay --argument 1"
```

# Run
To make the service launch the executable, just run from an elevated prompt:
```bash
net start ppl_runner
```
As a PPL service, when started ppl_runner will read the registry key, launch the child process,
also as PPL, then stop and exit. A successful launch will still say `the service failed to run`,
but if you check the return code with `sc query ppl_runner`, it should be 0, i.e. ERROR_SUCCESS.

# Cleanup/Removal
As the service is also Anti Malware PPL, it can only be stopped and deleted by a similarly high-level
process. However, we can use PPLRunner to remove itself, simply set the command in the registry key to be:
```powershell
C:\path\to\ppl_runner.exe remove
```
And run the Service. i.e. run:
```powershell
REG.exe ADD HKLM\SOFTWARE\PPL_RUNNER /ve /t REG_SZ /d "C:\path\to\ppl_runner.exe remove"
net start ppl_runner
```

# Restrictions
- This project only works in `testsigning` mode.
- `ppl_runner.exe install` must be re-run after every reboot
- The child binary must be signed with the same certificate as the service
- Any DLLs the binary loads must also be signed


# Debugging
Run Sysinternal's DBGView and log `Win32 Global`, filtering on `*[PPL_RUNNER]*`.
This will show all logs from the service and installer.


# Example uses
TBD - Sealighter blog

# Similar Projects
[James Forshaw](https://twitter.com/tiraniddo) created an awesome project to [inject code into existing PPL processes](https://googleprojectzero.blogspot.com/2018/10/injecting-code-into-windows-protected.html).

# Futher Reading and Thanks
Following [Alex Ionescu](https://twitter.com/aionescu) is probably the best way to learn more about ELAM and PPL.
Possibly start with this:
https://www.crowdstrike.com/blog/protected-processes-part-3-windows-pki-internals-signing-levels-scenarios-signers-root-keys/


Following [Matt Graeber](https://twitter.com/mattifestation) and [James Forshaw](https://twitter.com/tiraniddo) is another great way.

Massive thanks to Matt for the
[powershell script](https://gist.github.com/mattifestation/660d7e17e43e8f32c38d820115274d2e
) to get the 'To-Be-Signed' hash from a certificate.

James has [written](https://googleprojectzero.blogspot.com/2017/08/bypassing-virtualbox-process-hardening.html) a [lot](https://googleprojectzero.blogspot.com/2018/10/injecting-code-into-windows-protected.html) about PPL and its flaws.