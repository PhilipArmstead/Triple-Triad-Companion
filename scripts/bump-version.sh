#!/usr/bin/env bash
# Bumps the version by creating an annotated git tag. There is deliberately no version number
# stored anywhere in the tree: cmake/ProjectVersion.cmake derives it from `git describe`, so the
# tag is the single source of truth and it is impossible for a header, a .desktop file and an
# installer script to disagree about what is being built.
#
# Usage:
#   scripts/bump-version.sh patch|minor|major
#   scripts/bump-version.sh 2.1.0
#
# Then: git push --follow-tags

set -euo pipefail

cd "$(dirname "$0")/.."

die() { echo "bump-version: $*" >&2; exit 1; }

[[ $# -eq 1 ]] || die "expected one argument: patch|minor|major|<x.y.z>"

git rev-parse --git-dir >/dev/null 2>&1 || die "not a git repository"

# A tag describing a dirty tree points at a commit that nobody else can reproduce.
[[ -z "$(git status --porcelain)" ]] || die "working tree is dirty; commit or stash first"

current="$(git describe --abbrev=0 --match 'v[0-9]*' 2>/dev/null || echo 'v0.0.0')"
[[ ${current} =~ ^v([0-9]+)\.([0-9]+)\.([0-9]+)$ ]] || die "cannot parse current tag '${current}'"
major="${BASH_REMATCH[1]}"
minor="${BASH_REMATCH[2]}"
patch="${BASH_REMATCH[3]}"

case "$1" in
	major) next="$((major + 1)).0.0" ;;
	minor) next="${major}.$((minor + 1)).0" ;;
	patch) next="${major}.${minor}.$((patch + 1))" ;;
	[0-9]*.[0-9]*.[0-9]*)
		[[ $1 =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "'$1' is not a valid x.y.z version"
		next="$1"
		;;
	*) die "expected patch|minor|major|<x.y.z>, got '$1'" ;;
esac

tag="v${next}"
git rev-parse -q --verify "refs/tags/${tag}" >/dev/null && die "tag ${tag} already exists"

# Annotated, not lightweight: `git describe` ignores lightweight tags by default, so a lightweight
# tag here would leave the build reporting the previous version with a growing commit count.
git tag -a "${tag}" -m "Release ${next}"

echo "Tagged ${current} -> ${tag}"
echo "Push with:  git push --follow-tags"
