#!/bin/bash

# Shared by build.sh and archive.sh; callers set PROJECT_ROOT first.
release_version()
{
	local version="${1:-}" tag
	if [[ -z "$version" ]]; then
		if ! tag="$(git -C "$PROJECT_ROOT" describe --tags --abbrev=0 --match '[0-9]*.[0-9]*.[0-9]*')"; then
			printf 'No release tag found; provide a release version explicitly.\n' >&2
			return 1
		fi
		version="$tag"
		if [[ "$(git -C "$PROJECT_ROOT" rev-parse HEAD)" != "$(git -C "$PROJECT_ROOT" rev-parse "$tag^{commit}")" ]]; then
			version="$version-wip-$(git -C "$PROJECT_ROOT" rev-parse --short HEAD)"
		fi
		if [[ -n "$(git -C "$PROJECT_ROOT" status --porcelain)" ]]; then
			version="$version-dirty"
		fi
	fi
	if [[ ! "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+([-+][A-Za-z0-9.+-]+)?$ ]]; then
		printf 'Provide a release version such as 1.2.0 or 1.2.0-rc1.\n' >&2
		return 1
	fi
	printf '%s\n' "$version"
}
