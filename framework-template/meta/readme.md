fwk-name test service and framework-policy

This packaging can be used as boilerplate. The example code provides a simple
UNIX abstract socket server and a cli command to access it.

For strict confinement that works with the example code, use:
  security-policy:
    apparmor: meta/svc.apparmor
    seccomp: meta/svc.seccomp

For strict confinement for new projeccts, use (adjusting as necessary):
  security-policy:
    apparmor: meta/svc.apparmor.boilerplate
    seccomp: meta/svc.seccomp.boilerplate

To use permissive confinement (for testing purposes only):
  security-policy:
    apparmor: meta/svc.apparmor.unconfined
    seccomp: meta/svc.seccomp.unconfined

For more information, see:
 * https://developer.ubuntu.com/en/snappy/guides/security-policy/
 * https://developer.ubuntu.com/en/snappy/guides/filesystem-layout/
 * https://developer.ubuntu.com/en/snappy/guides/frameworks/
 * https://wiki.ubuntu.com/SecurityTeam/Specifications/SnappyConfinement
 * https://wiki.ubuntu.com/SecurityTeam/Specifications/SnappyConfinement/DevelopingFrameworkPolicy
