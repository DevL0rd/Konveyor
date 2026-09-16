# Publishing to the KDE Store

Konveyor is a compiled KWin plugin. KWin refuses to load a plugin built against a different KWin version, so the store entry points at the source and the install script instead of shipping a binary.

## One-time setup

1. Create an account at https://store.kde.org (the same account works on all OpenDesktop sites).
2. Click **Add Product** and pick the category **KWin Effects Plasma 6** (category id 719).
3. Fill in the listing from `metadata.json` in this directory:
   - **Title:** Konveyor
   - **Summary:** Scrollable tiling for KDE Plasma
   - **Description:** the "What it does" section of the top-level `README.md`
   - **License:** GPL-3.0-or-later
   - **Homepage:** the GitHub repository
   - **Tags:** the `tags` list from `metadata.json`
4. Upload `docs/screenshot.png` as the product image.
5. Under **Files**, add a link to the latest GitHub release archive rather than uploading a binary, and put the three install commands from the README in the download description.

## For each release

Run `./release.sh <version>`, which tags the release and lets the CI build and publish the archive. Then edit the store entry:

1. Add the new release archive link under **Files**.
2. Paste the release notes into **Changelog**.

The store has an upload API (OCS `content/add`), but it needs a personal API key tied to the account, so releases are published by hand until an account exists.
