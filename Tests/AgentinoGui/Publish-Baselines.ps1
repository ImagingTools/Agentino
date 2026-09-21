# SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-ImtCore-Commercial
<#
.SYNOPSIS
    Opens a pull request with the screenshot baselines a -UpdateSnapshots run just produced.

.DESCRIPTION
    Screenshot baselines have to be minted on the machine that compares them, which is the build
    agent - see Run-CiTests.ps1's -UpdateSnapshots. This carries the result back: it branches,
    commits whatever changed under the baseline directory, pushes, and opens a PR.

    A PR rather than a push straight to the target branch, deliberately. A run with the flag set is
    green by construction, because nothing is compared - so a direct push would let an accidentally
    ticked build silently overwrite baselines, including ones that had just caught a regression.
    A PR keeps that visible and still costs one click.

    Does nothing at all, successfully, when there is no token or when the run changed no baseline.
    That is the normal case: every build carries this step, only a ticked one has anything to say.
#>
param(
    [Parameter(Mandatory = $true)][string]$RepoRoot,

    # Relative to $RepoRoot, e.g. "Tests/AgentinoGui/tests/__screenshots__".
    [Parameter(Mandatory = $true)][string]$BaselineDir,

    # A token with push rights. Empty means "not configured" and this script then exits quietly -
    # the baselines are still published as a build artifact either way.
    #
    # It cannot be taken from the VCS root, though an equivalent token already lives there. TeamCity
    # hands a build the root's url, username, branch and authMethod, but deliberately NOT its
    # secure:password - verified against a real build's resulting-properties. So this is its own
    # password parameter, defined once per project and inherited by the configs.
    [string]$Token = $env:GUI_BASELINE_TOKEN,

    # GitHub ignores the user name when the password is a token, so the default suits it. Named
    # explicitly for hosts that do care, and so the URL reads as what it is.
    [string]$Username = $(if ($env:GUI_BASELINE_USER) { $env:GUI_BASELINE_USER } else { "x-access-token" }),

    [string]$BaseBranch = "main",
    [string]$BuildNumber = $(if ($env:BUILD_NUMBER) { $env:BUILD_NUMBER } else { Get-Date -Format "yyyyMMdd-HHmmss" }),
    [string]$AuthorName = "TeamCity",
    [string]$AuthorEmail = "teamcity@imagingtools.local"
)

$ErrorActionPreference = "Stop"

function Write-Step($text) { Write-Host "`n=== $text ===" -ForegroundColor Cyan }

Push-Location $RepoRoot
try {
    if (-not $Token) {
        Write-Host "No baseline token configured - leaving the refreshed baselines in the build artifact only."
        exit 0
    }

    $full = Join-Path $RepoRoot $BaselineDir
    if (-not (Test-Path $full)) { throw "Baseline directory not found: $full" }

    # --porcelain is the only reliable "did anything change" here: a -UpdateSnapshots run rewrites
    # every file, so timestamps move even when the bytes do not.
    $changed = @(& git status --porcelain -- $BaselineDir)
    if ($changed.Count -eq 0) {
        Write-Host "Baselines are byte-identical to the committed ones - nothing to open a PR for."
        exit 0
    }
    Write-Host "$($changed.Count) baseline file(s) differ from $BaseBranch."

    $branch = "ci/gui-baselines-$BuildNumber"
    Write-Step "Committing to $branch"

    # The agent checks out a revision, not a branch, so this starts from a detached HEAD.
    & git checkout -b $branch
    if ($LASTEXITCODE -ne 0) { throw "git checkout -b $branch failed (exit $LASTEXITCODE)" }

    & git -c "user.name=$AuthorName" -c "user.email=$AuthorEmail" add -- $BaselineDir
    if ($LASTEXITCODE -ne 0) { throw "git add failed (exit $LASTEXITCODE)" }

    $subject = "Re-mint GUI screenshot baselines on the build agent"
    $bodyText = @"
$subject

Produced by build $BuildNumber with the "Re-mint GUI screenshot baselines"
parameter set, so these images come from the machine that compares them.

$($changed.Count) file(s) changed.
"@
    & git -c "user.name=$AuthorName" -c "user.email=$AuthorEmail" commit -m $bodyText
    if ($LASTEXITCODE -ne 0) { throw "git commit failed (exit $LASTEXITCODE)" }

    Write-Step "Pushing $branch"
    $origin = (& git remote get-url origin).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $origin) { throw "could not read the origin remote" }
    if ($origin -notmatch '^https://github\.com/([^/]+)/([^/.]+)') {
        throw "origin is not an https github remote, so the token cannot be applied: $origin"
    }
    $owner = $Matches[1]
    $repo = $Matches[2]
    # The token goes in the URL for this one push only, never into the stored remote.
    $authUrl = "https://${Username}:$Token@github.com/$owner/$repo.git"
    & git push $authUrl "${branch}:${branch}"
    if ($LASTEXITCODE -ne 0) { throw "git push failed (exit $LASTEXITCODE)" }

    Write-Step "Opening the pull request"
    $payload = @{
        title = $subject
        head  = $branch
        base  = $BaseBranch
        body  = "Minted by build ``$BuildNumber`` on the agent, with the *Re-mint GUI screenshot baselines* parameter set.`n`nBaselines have to come from the machine that compares them - a browser update on either side is enough to fail an otherwise identical page - so these were produced where they will be used.`n`n**Look at the diff before merging.** A run with that parameter set is green by construction, because nothing is compared: if a real regression was on screen, it is in these images now."
    } | ConvertTo-Json

    $headers = @{
        Authorization = "Bearer $Token"
        Accept        = "application/vnd.github+json"
        "User-Agent"  = "imt-gui-baselines"
    }
    try {
        $pr = Invoke-RestMethod -Method Post -Uri "https://api.github.com/repos/$owner/$repo/pulls" -Headers $headers -Body $payload -ContentType "application/json"
        Write-Host "Pull request: $($pr.html_url)" -ForegroundColor Green
    }
    catch {
        # The branch is pushed either way, so this is recoverable by hand - say so rather than
        # failing the build over the PR call alone.
        Write-Host "Pushed $branch, but opening the pull request failed: $($_.Exception.Message)" -ForegroundColor Yellow
        Write-Host "Open it by hand from $branch." -ForegroundColor Yellow
    }
}
finally {
    Pop-Location
}
