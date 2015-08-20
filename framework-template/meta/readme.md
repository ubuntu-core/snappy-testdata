fwk-name test service and framework-policy

This can be used as a template.

For strict confinement, use (adjusting as necessary):
  security-policy:
    apparmor: meta/svc.apparmor
    seccomp: meta/svc.seccomp

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
