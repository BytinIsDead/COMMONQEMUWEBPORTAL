# Extreme QEMU Web Manager — AGPL-3.0-or-later
# Copyright (C) 2026 Extreme QEMU Web Manager contributors.
# Licensed under the GNU Affero General Public License version 3 or later.

$ErrorActionPreference = 'Stop'
dotnet restore
dotnet build --configuration Release
dotnet publish --configuration Release --self-contained false --output dist
