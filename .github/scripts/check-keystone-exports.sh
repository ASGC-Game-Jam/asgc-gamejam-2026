#!/usr/bin/env bash
# Flags Unreal assets a Pull Request changes without updating their Keystone export.
#
# Keystone reviews an asset by diffing BlueprintGraphs/<path>.bpgraph.json, which the
# KeystoneBlueprintExport plugin writes for every Content asset when it is saved. If the .uasset
# changed but its export did not, Keystone can only show the old data. This catches that before
# review. Modified assets are only flagged when they already have an export, so assets from
# before the plugin exported every asset type aren't reported until someone exports them.
#
# Usage: check-keystone-exports.sh <base-sha> <head-sha>
# Reads git objects only, so it needs no LFS content. Exits 1 when something is flagged.
set -euo pipefail

base="$1"
head="$2"
mb="$(git merge-base "$base" "$head")"

# Mirrors blueprintExportPathForUasset in Keystone (@keystone/types) for a project at the repo
# root: Content/X/Y.uasset and Plugins/P/Content/X/Y.uasset map into BlueprintGraphs/.
# One-File-Per-Actor packages have no export.
export_path_for() {
  local p="$1" rest
  case "$p" in
    Content/__ExternalActors__/* | Content/__ExternalObjects__/*) return 1 ;;
    Content/*) rest="${p#Content/}" ;;
    Plugins/*/Content/*)
      local plugin="${p#Plugins/}"
      plugin="${plugin%%/*}"
      rest="$plugin/${p#Plugins/"$plugin"/Content/}"
      ;;
    *) return 1 ;;
  esac
  printf 'BlueprintGraphs/%s.bpgraph.json\n' "${rest%.*}"
}

exists_at() { git cat-file -e "$1:$2" 2>/dev/null; }

changed="$(git diff --name-only --no-renames "$mb" "$head")"

stale=()
missing=()
while IFS=$'\t' read -r status path; do
  [[ "$path" =~ \.(uasset|umap)$ ]] || continue
  exp="$(export_path_for "$path")" || continue
  grep -qxF "$exp" <<<"$changed" && continue

  if [[ "$status" == "M" ]] && { exists_at "$mb" "$exp" || exists_at "$head" "$exp"; }; then
    stale+=("$path|$exp")
  elif [[ "$status" == "A" ]] && ! exists_at "$head" "$exp"; then
    missing+=("$path|$exp")
  fi
done < <(git diff --name-status --no-renames --diff-filter=AM "$mb" "$head")

summary="${GITHUB_STEP_SUMMARY:-/dev/stdout}"

if ((${#stale[@]} == 0 && ${#missing[@]} == 0)); then
  echo "Every changed asset has an up-to-date Keystone export."
  echo "✅ Every changed asset has an up-to-date Keystone export." >>"$summary"
  exit 0
fi

{
  echo "## ⚠️ Keystone exports not updated"
  echo
  echo "Keystone shows what changed in an asset by comparing its file in \`BlueprintGraphs/\`."
  echo "These assets changed but their export did not, so reviewers can't see what changed."
  echo
  echo "**To fix:** open the project on this branch, run **Keystone ▸ Export Assets for Keystone…**,"
  echo "then **Keystone ▸ Commit & Push Keystone Exports…** (or commit the \`BlueprintGraphs/\` changes yourself)."
  echo
  echo "If the only change was a resave or recompile, the export can legitimately stay the same."
  echo "Say so in the Pull Request and a reviewer can ignore this check."
  echo
  if ((${#stale[@]})); then
    echo "### Changed, export not updated"
    echo
    for e in "${stale[@]}"; do echo "- \`${e%%|*}\` → \`${e#*|}\`"; done
    echo
  fi
  if ((${#missing[@]})); then
    echo "### New asset, no export yet"
    echo
    for e in "${missing[@]}"; do echo "- \`${e%%|*}\` → \`${e#*|}\`"; done
    echo
  fi
} >>"$summary"

for e in "${stale[@]}"; do
  echo "::warning file=${e%%|*}::Changed, but its Keystone export ${e#*|} was not updated. Re-export (Keystone ▸ Export Assets for Keystone) and commit BlueprintGraphs/."
done
for e in "${missing[@]}"; do
  echo "::warning file=${e%%|*}::New asset with no Keystone export (${e#*|}). Re-export (Keystone ▸ Export Assets for Keystone) and commit BlueprintGraphs/."
done
exit 1
