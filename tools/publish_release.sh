#!/usr/bin/env bash
# Publish a folder of files as a GitHub Release, in a way that survives a flaky API.
#
#   tools/publish_release.sh <tag> <folder> [--dry-run]
#
# CI runs this on a version tag, with the bundled firmware images. It:
#   1. creates the release as a DRAFT, so nothing is public until every file is attached
#      (a v0.* tag is marked a pre-release);
#   2. uploads the files one at a time, retrying each a few times with a growing pause;
#   3. checks every file arrived, then publishes the draft.
#
# It is safe to run again after a failure: it reuses an existing draft (--clobber replaces
# files already there), and does nothing to a release that is already published.
# --dry-run stops before publishing, leaving the draft for you to inspect and delete.
#
# Needs the gh CLI, authenticated (GH_TOKEN) with write access, and the repo (GH_REPO or a clone).
set -euo pipefail

tag=${1:?usage: publish_release.sh <tag> <folder> [--dry-run]}
folder=$(cd "${2:?usage: publish_release.sh <tag> <folder> [--dry-run]}" && pwd)
dry_run=${3:-}
cd "$(dirname "$0")/.."

retry() {
  local attempt=0
  until "$@"; do
    attempt=$((attempt + 1))
    if ((attempt >= 4)); then return 1; fi
    echo "  attempt $attempt failed; retrying in $((attempt * 5))s" >&2
    sleep $((attempt * 5))
  done
}

# What does GitHub already have for this tag? (A draft does not show up by tag, so list.)
state=$(gh release list --limit 100 --json tagName,isDraft \
  --jq "map(select(.tagName == \"$tag\")) | if length == 0 then \"none\" elif .[0].isDraft then \"draft\" else \"published\" end")

if [[ "$state" == "published" ]]; then
  echo "release $tag is already published; nothing to do"
  exit 0
fi

if [[ "$state" == "none" ]]; then
  notes=$(mktemp)
  flags=(--draft --title "$tag" --generate-notes)
  if [[ "$tag" == v0.* ]]; then
    flags+=(--prerelease)
    echo "> **Pre-release.** Read [Not yet verified on real hardware](https://github.com/${GITHUB_REPOSITORY:-ImranNZ/boost-gauge}#not-yet-verified-on-real-hardware) in the README before using these images." >"$notes"
    echo >>"$notes"
  fi
  cat .github/release-notes.md >>"$notes"
  # In CI the tag exists; for a dry run on a made-up tag there is nothing to verify.
  if git rev-parse -q --verify "refs/tags/$tag" >/dev/null; then flags+=(--verify-tag); fi
  echo "creating draft release $tag"
  retry gh release create "$tag" "${flags[@]}" --notes-file "$notes"
else
  echo "reusing the existing draft for $tag"
fi

files=("$folder"/*)
for file in "${files[@]}"; do
  echo "uploading $(basename "$file")"
  retry gh release upload "$tag" "$file" --clobber
done

attached=$(gh release view "$tag" --json assets --jq '.assets | length')
if ((attached != ${#files[@]})); then
  echo "expected ${#files[@]} files on the release but it has $attached; leaving it as a draft" >&2
  exit 1
fi

if [[ "$dry_run" == "--dry-run" ]]; then
  echo "dry run: $attached files attached; left as a draft (delete it with: gh release delete $tag)"
  exit 0
fi

retry gh release edit "$tag" --draft=false
echo "published $tag with $attached files"
