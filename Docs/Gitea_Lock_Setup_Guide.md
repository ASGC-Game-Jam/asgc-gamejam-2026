# ASGC Game Jam 2026: Gitea + Unreal setup guide

Set up Git LFS file locking so you can check out assets in Unreal.

## Pick your path

🔵 **Already set up?** Follow **only the blue steps.** This also covers anyone affected by the Gitea reset.

⚪ **First time?** Follow **all the steps**, blue and grey, in order. Start at Step 1.

> ⚠️ **Take this with a grain of salt.** I summarized what worked for me and had AI (Claude) create this guide, with steps, pictures, fixes and copy-pasteable terminal commands. I haven't double-checked everything. If it disagrees with the official docs, trust the official docs.

> ⏳ **Got "too many authentication attempts"?** This guide isn't a guaranteed fix, but it gives you a better chance of not hitting it again. If you got the error recently, **wait at least 15 minutes** before trying this. You might still be locked out.

> ℹ️ **About the reset:** Gitea was wiped and restored after a security incident on 4 November. Everyone has to log in again and set a new password. Details are in the Discord #general announcement.

**Links:** [SETUP.md](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/blob/main/Docs/SETUP.md) · [CONTRIBUTING.md](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/blob/main/CONTRIBUTING.md) · [Gitea 2FA.md](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/blob/main/Docs/Gitea%202FA.md) · [Git](https://git-scm.com/install/) · [Git LFS](https://git-lfs.com/) · [Gitea](https://asgc.freemyip.com/)

## ⚪ Step 1: Install the requirements first

*Grey step: first-time setup only. Skip it if you already have the repository set up.*

Make sure these are installed before you do anything else:

- [Git](https://git-scm.com/install/) and [Git LFS](https://git-lfs.com/)
- **Unreal Engine 5.7.4**, the version `SETUP.md` asks for
- **Visual Studio 2022** or **Rider**, set up for Unreal Engine C++ development

Check that Git LFS is installed, and initialize it if needed:

```powershell
git lfs version
git lfs install
```

## 🔵 Step 2: Log in to Gitea with GitHub

*Blue step: do this one whether you already have the repository set up or you're setting up for the first time.*

Go to [asgc.freemyip.com](https://asgc.freemyip.com/) and choose **Sign in with GitHub**. After the reset, Gitea asks you to set a new password. If it doesn't, you can set one under **Settings → Account**. If you signed in with GitHub and don't remember your Gitea password, that's okay. You'll use a token instead (Steps 3 and 10). The passcode screen only appears if you have already set up two-factor authentication (2FA) on your Gitea account. If you haven't, you log straight in with GitHub.

## 🔵 Step 3: Generate a token

*Blue step: do this one whether you already have the repository set up or you're setting up for the first time.*

Click your avatar in the top right, then **Settings**, then go to **Applications → Generate New Token**.

- Give it a name (for example `pc-git`).
- Set **repository** to **Read and Write**.
- Click **Generate Token** and copy the token into Notepad right away. Gitea only shows it once.
- Don't paste it into chats, screenshots or commits.

> **Already have a token?** If it still works, you can keep using it. If checking out or locking fails after the reset, generate a new one.

> **Good to know:** once you have a token, you can use it instead of your password. You don't need the password anymore.

> **If you enabled 2FA on Gitea,** plain username and password Git authentication stops working, so use a token. See [Gitea 2FA.md](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/blob/main/Docs/Gitea%202FA.md) in the repository for setting up the token in both Bash (WSL) and PowerShell.

## ⚪ Step 4: Find the repository link and clone it

*Grey step: first-time setup only. Skip it if you already have the repository set up.*

> **Note:** The folder `C:\Game Projects` is the author's own. Pick any folder you like, ideally a short path without odd characters, and use that wherever you see it. In commands, `C:\YOUR\FOLDER` is a placeholder for your own folder.

The project repository is `ASGC-Game-Jam/asgc-gamejam-2026` on GitHub. If you don't have the link handy:

- Open the repository page: `https://github.com/ASGC-Game-Jam/asgc-gamejam-2026`.
- Click the green **Code** button, stay on the **Local** tab, and under **HTTPS** click the copy button next to the URL.

```powershell
https://github.com/ASGC-Game-Jam/asgc-gamejam-2026.git
```

Then clone it with GitHub Desktop:

- Click **Add → Clone repository...**.

You can pick the repository in two ways:

- **GitHub.com tab, if you're connected to the project.** If your GitHub account is connected to the project, the repository shows up in the list under **ASGC-Game-Jam**. Select **ASGC-Game-Jam/asgc-gamejam-2026**. Don't pick a copy listed under your own name.
- **URL tab, which works either way.** If you don't see it in the list, open the **URL** tab and paste the repository link from above.
- Set the **Local path** to the folder where you want the project (the author used `C:\Game Projects`). GitHub Desktop creates an `asgc-gamejam-2026` folder inside it.
- Click **Clone** and wait. The first clone is large, because the project assets download through Git LFS.

> **If you see "This folder contains files. Git can only clone to empty folders"**, the destination already holds files, for example from an earlier clone. Pick a different or empty folder.

If you prefer the terminal, the same thing is:

```powershell
git clone https://github.com/ASGC-Game-Jam/asgc-gamejam-2026.git
```

If the assets didn't download automatically, run this from inside the repo folder:

```powershell
git lfs pull
```

If Git asks for credentials during the clone, see Step 10 (use your token as the password).

## 🔵 Step 5: Make sure your paths are correct

*Blue step: do this one whether you already have the repository set up or you're setting up for the first time.*

This is the most common cause of instant failures. With a wrong path, Git and Unreal fail every time, whatever credentials you use.

> **Replace the example path.** Commands in this guide use `C:\YOUR\FOLDER\asgc-gamejam-2026` as a placeholder. Swap `C:\YOUR\FOLDER` for the folder you cloned the repository into. The author's own clone is in `C:\Game Projects`, but yours will be different. Tip: open the repo folder in File Explorer, click the address bar and copy the path.

- **Work in the right clone.** It must be cloned from the project (`ASGC-Game-Jam/asgc-gamejam-2026`), not a stray local copy. If you have several copies, delete or ignore the extras.

```powershell
cd "C:\YOUR\FOLDER\asgc-gamejam-2026"
git rev-parse --show-toplevel
git remote -v
```

- **Run commands from inside the repo folder.** From somewhere like `C:\Users\yourname`, Git says `fatal: not in a git directory`. Put quotes around paths with spaces.
- **Open the project from that same clone.** Launch `ProjectAtlantis.uproject` from the repo folder so Unreal's revision control points at the right repo.
- **Use repo-relative paths with forward slashes** and exact capitalization in lock commands. Don't use full Windows paths or backslashes.

```powershell
git lfs lock Content/ProjectAtlantis/UI/WBP_PauseMenu.uasset
```

## ⚪ Step 6: Turn on lock verification

*Grey step: first-time setup only. Skip it if you already have the repository set up.*

Open PowerShell **inside the repo folder** and run the commands below. **Replace `C:\YOUR\FOLDER` with the folder you cloned into.** The example is a placeholder, not a real path. Tip: open the repo folder in File Explorer, click the address bar and copy the path.

```powershell
cd "C:\YOUR\FOLDER\asgc-gamejam-2026"
git config lfs.locksverify true
git config --get lfs.locksverify
```

The last command should print `true`.

Running this outside the repo gives the error below. If you see it, you're in the wrong folder. See Step 5.

## 🔵 Step 7: Check the setup in PowerShell

*Blue step: do this one whether you already have the repository set up or you're setting up for the first time.*

From the repo folder, list the current locks. It should print a list with no errors:

```powershell
git lfs locks
```

Then check your permissions on the repository. Replace `YOUR_GITEA_USERNAME` with your Gitea username, and paste your token at the password prompt:

```powershell
$url = "https://asgc.freemyip.com/api/v1/repos/ASGC/asgc-gamejam-2026"
(curl.exe -s -u YOUR_GITEA_USERNAME $url | ConvertFrom-Json).permissions
```

The result should show `push: True`. If it says `False`, a token can't fix it. A Gitea admin has to give you write access, because creating locks needs write permission.

## ⚪ Step 8: Open the project in Unreal

*Grey step: first-time setup only. Skip it if you already have the repository set up.*

- Open the repo folder in File Explorer and double-click **ProjectAtlantis.uproject**.
- If Unreal asks to build the project files, choose **Yes**.
- The first launch can take several minutes while Unreal generates project files, compiles shaders and prepares the project.

## 🔵 Step 9: Connect Unreal to revision control

*Blue step: do this one whether you already have the repository set up or you're setting up for the first time.*

After the Gitea wipe, Unreal is probably disconnected from revision control, even if you already had the project set up. Connect it again so you can check out assets:

- Click **Revision Control**. It is the button at the very **bottom right corner** of the editor, next to **All Saved** (look for the small branch icon). If your window is small, the name may be cut off. Then choose **Connect to Revision Control**.
- Set **Provider** to **Git LFS 2**. For the author, the rest of the window filled in automatically once Git LFS 2 was selected: the Git path (for example `C:\Program Files\Git\bin\git.exe`), the repository root (your clone folder), and the user name and email. Check that the Git path and repository root look right for your setup. Use the existing repo and don't initialize a new one.
- Check that **Uses Git LFS** is ticked, and look at the field next to it. For the author it also filled in on its own, with the name from Git config. `SETUP.md` says to set this to your Gitea identity, so change it to your **Gitea username** if it doesn't match.
- Click **Accept Settings**. A message saying "Connection to revision control was successful" appears when it works.
- If Git LFS 2 isn't an option, enable the plugin under Plugins and restart the editor.

When you first check out an asset, the Git Credential Manager popup may appear. Enter your Gitea username and paste your token as the password (Step 10). If checkout fails with "too many authentication attempts", check Steps 5, 7 and 10 again.

## 🔵 Step 10: Log in with the token, not your password, and go slowly

*Blue step: do this one whether you already have the repository set up or you're setting up for the first time.*

> **Signed in with GitHub and don't remember your Gitea password? That's okay.** As long as you can create a token (Step 3), use the token instead of the password. Once you have a token, you don't need the password anymore.

- When the credentials popup appears, enter your **Gitea username** and paste the **token** as the password.
- The popup is the **Git Credential Manager** window (shown below), not a GitHub Desktop window. In it you will see the long token when you paste it. In a terminal, the password may not show while you type or paste. The author hasn't set it up through the terminal, so doesn't know the details of how it behaves there. In PowerShell, right-click pastes.
- **Wait for the popup to fully appear and enter it once.** Every rejected or rushed attempt counts toward "too many authentication attempts".
- To test, run the lock command below once, or use Check Out in Unreal.
- If you get the error, stop and wait a minute or two before retrying.
- **The popup may come back even if you were patient.** It can also appear when you fetch the latest in GitHub Desktop. If it shows up again, wait a moment for GitHub in the background to catch up, then sign in again.
- **An old saved login can get in the way.** A password you saved before the reset likely won't work anymore. If an old or wrong login is saved, delete the `asgc.freemyip.com` entry in **Credential Manager → Windows Credentials**, then sign in again with your token.
- Close and reopen Unreal afterward so it picks up the saved login.

Test command (swap in the asset you want to check out, using a repo-relative path):

```powershell
git lfs lock Content/ProjectAtlantis/UI/WBP_PauseMenu.uasset
```

## 🔵 Step 11: If you use 2FA, store your token as a Git credential

*Blue step: do this one whether you already have the repository set up or you're setting up for the first time.*

This step is optional. If you haven't enabled 2FA on Gitea, skip it. With 2FA on, plain username and password login over HTTPS stops working for Git and Git LFS, so you store your access token as your Git credential instead. The repository's [Gitea 2FA.md](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/blob/main/Docs/Gitea%202FA.md) has the exact commands, for both PowerShell and Bash (WSL or Git Bash):

- Create an access token (Step 3).
- Store the token as your Git credential using the commands in Gitea 2FA.md for your shell. Replace the username and token placeholders in them with your own.
- Verify it was stored. The `git credential fill` command in that doc should print your credential straight back without asking you for anything.
- That check prints your token in plain text, so do it somewhere private and clear your scrollback afterward.
- If you also use WSL, check whether it shares the Windows credential. Gitea 2FA.md explains how.

## 🔵 Step 12: Check out assets before you edit them

*Blue step: do this one whether you already have the repository set up or you're setting up for the first time.*

> **Lock AND use a branch.** Locking the files you're working on isn't enough on its own. As `CONTRIBUTING.md` says, also make your changes on your own branch created from `main`, then open a Pull Request. Locks stop two people editing the same binary asset. The branch keeps your work separate until it has been reviewed.

- Not on a branch yet? In GitHub Desktop, click **Current branch**, then **New Branch**, and base it on `main`.
- In the Content Browser, right-click the asset and choose **Revision Control → Check Out**.
- Hover over an asset to see its status, including who has it checked out. If someone else has it, talk to them before you edit it.
- Auto-checkout is not enabled, so always check out first. New files must be added explicitly with `git add`.
- Keep the locks on the assets you're changing until your Pull Request is merged, then release them. Never force-unlock someone else's work.
- If you locked an asset only to test, release it right away so you don't block teammates:

```powershell
git lfs unlock Content/ProjectAtlantis/UI/WBP_PauseMenu.uasset
```

Unreal's built-in version control is easier to use than the terminal for everyday work. The terminal commands are for troubleshooting or extra technical work.

## ⚪ Step 13: Verify your setup

*Grey step: first-time setup only. Skip it if you already have the repository set up.*

Before making changes, check that:

- The project opens without errors and compiles successfully.
- You can launch the editor and load the default map.
- Git LFS assets downloaded successfully.

If you plan to edit shared binary assets, check that the lock server is reachable:

```powershell
git lfs locks
```

If anything fails, ask for help before continuing.

## ⚪ Step 14: Start working

*Grey step: first-time setup only. Skip it if you already have the repository set up.*

This is the workflow from [CONTRIBUTING.md](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/blob/main/CONTRIBUTING.md), in order:

- **Sync** your local repository: pull the latest changes from `main`.
- **Create a new branch from `main`** and work on it, not on `main`.
- **Check out (lock)** the assets you'll edit, on your branch (Step 12).
- Make your changes and test them.
- **Commit** your work, then open a **Pull Request**. Keep it focused on a single feature or fix.
- Release your locks after your Pull Request is merged. The plugin doesn't watch PR status.
- More details: [Git Hygiene](https://github.com/ASGC-Game-Jam/asgc-gamejam-2026/blob/main/Docs/Git%20Hygiene.md).

*Screenshots are in the PDF version of this guide.*
