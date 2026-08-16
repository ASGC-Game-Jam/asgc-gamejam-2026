# In Case You Enabled 2FA on Gitea

For initial setup, cloning, and project requirements, see [SETUP.md](./SETUP.md)

**Note**: Ignore this guide if Gitea 2FA is disabled.

If you enable 2FA on Gitea, plain username/password authentication over HTTPS
(used by Git and Git LFS) stops working. Instead, generate an access token and
store it as your Git credential.

1. Create an access token in the [Applications](https://asgc.freemyip.com/user/settings/applications)
   settings tab via the [user interface](https://docs.gitea.com/development/api-usage/#user-interface).
2. Store the token as your Git credential, then immediately verify it was
   stored, using the commands for your shell below. Replace
   `your-gitea-username` and `your-personal-access-token` with your actual
   values.

Bash (WSL / Git Bash):

```bash
printf "protocol=https\nhost=asgc.freemyip.com\nusername=your-gitea-username\npassword=your-personal-access-token\n\n" | git credential approve
printf "protocol=https\nhost=asgc.freemyip.com\n\n" | git credential fill
```

PowerShell:

```powershell
@"
protocol=https
host=asgc.freemyip.com
username=your-gitea-username
password=your-personal-access-token

"@ | git credential approve

@"
protocol=https
host=asgc.freemyip.com

"@ | git credential fill
```

Both end with `git credential fill`, which should immediately print your
stored credential back without prompting:

```text
protocol=https
host=asgc.freemyip.com
username=your-gitea-username
password=your-personal-access-token
```

This prints your token in plain text, so run it somewhere private and clear
your scrollback afterward.

## Do WSL and PowerShell Share the Same Credential?

They can, depending on how WSL's Git is configured. Check both:

```
git config --show-origin --get-all credential.helper
```

Example:
```
PS C:\Dev\asgc-gamejam-2026> git config --show-origin --get-all credential.helper
file:C:/Program Files/Git/etc/gitconfig manager
file:C:/Users/dulta/.gitconfig  manager
PS C:\Dev\asgc-gamejam-2026> wsl
To run a command as administrator (user "root"), use "sudo <command>".
See "man sudo_root" for details.

dulta@Dansktop:/mnt/c/Dev/asgc-gamejam-2026$ git config --show-origin --get-all credential.helper
file:/home/dulta/.gitconfig     /mnt/c/Program\ Files/Git/mingw64/bin/git-credential-manager.exe
dulta@Dansktop:/mnt/c/Dev/asgc-gamejam-2026$ 
```

- **If WSL's helper points at the Windows GCM executable** (something like
  `/mnt/c/Program Files/Git/mingw64/bin/git-credential-manager.exe`), WSL
  isn't using a separate Linux credential store — it's invoking the same
  Windows binary PowerShell uses, which reads and writes the same Windows
  Credential Manager entry. Storing the token once from either shell covers
  both, and rejecting/updating it in one is immediately visible in the other.
- **If WSL's helper is something else** (`store`, `cache`, `libsecret`, or
  unset), WSL keeps its own independent credential store and you need to
  store the token separately in each shell.

To point WSL at the Windows store, set:

```bash
git config --global credential.helper "/mnt/c/Program Files/Git/mingw64/bin/git-credential-manager.exe"
```

A couple of caveats even when sharing is set up:

- Entries are keyed by `protocol` and `host` (and `path`, if
  `credential.useHttpPath` is set), so both shells need to hit the exact same
  host string (`asgc.freemyip.com`) to see the same entry.
- This only applies to HTTPS. If either shell clones or pushes over SSH,
  credential storage isn't involved — SSH uses keys instead.
