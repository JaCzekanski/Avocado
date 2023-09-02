# Author: Sankarsan Kampa (a.k.a. k3rn31p4nic)
# License: MIT

$STATUS=$args[0]
$WEBHOOK_URL=$args[1]

if (!$WEBHOOK_URL) {
  Write-Output "WARNING!!"
  Write-Output "You need to pass the WEBHOOK_URL environment variable as the second argument to this script."
  Write-Output "For details & guide, visit: https://github.com/DiscordHooks/appveyor-discord-webhook"
  Exit
}

Write-Output "[Webhook]: Sending webhook to Discord..."

Switch ($STATUS) {
  "success" {
    $EMBED_COLOR=3066993
    $STATUS_MESSAGE="Passed"
    Break
  }
  "failure" {
    $EMBED_COLOR=15158332
    $STATUS_MESSAGE="Failed"
    Break
  }
  default {
    Write-Output "Default!"
    Break
  }
}
$AVATAR="https://upload.wikimedia.org/wikipedia/commons/thumb/b/bc/Appveyor_logo.svg/256px-Appveyor_logo.svg.png"

if (!$env:APPVEYOR_REPO_COMMIT) {
  $env:APPVEYOR_REPO_COMMIT="$(git log -1 --pretty="%H")"
}

$COMMIT_AUTHOR="$env:APPVEYOR_REPO_COMMIT_AUTHOR committed"
$COMMIT_SUBJECT="$(git log -1 "$env:APPVEYOR_REPO_COMMIT" --pretty="%s")"
$COMMIT_MESSAGE="$(git log -1 "$env:APPVEYOR_REPO_COMMIT" --pretty="%b")"

if ($env:APPVEYOR_PULL_REQUEST_NUMBER) {
  $COMMIT_SUBJECT="PR #$env:APPVEYOR_PULL_REQUEST_NUMBER - $env:APPVEYOR_PULL_REQUEST_TITLE"
  $URL="https://github.com/$env:APPVEYOR_REPO_NAME/pull/$env:APPVEYOR_PULL_REQUEST_NUMBER"
}
else {
  $URL=""
}

$BUILD_VERSION = [uri]::EscapeDataString($env:APPVEYOR_BUILD_VERSION)
$TIMESTAMP="$(Get-Date -format s)Z"
$WEBHOOK_DATA="{
  ""avatar_url"": ""$AVATAR"",
  ""embeds"": [ {
    ""color"": $EMBED_COLOR,
    ""author"": {
      ""name"": ""Job #$env:APPVEYOR_JOB_NUMBER (Build #$env:APPVEYOR_BUILD_NUMBER) $STATUS_MESSAGE - $env:APPVEYOR_REPO_NAME"",
      ""url"": ""https://ci.appveyor.com/project/$env:APPVEYOR_ACCOUNT_NAME/$env:APPVEYOR_PROJECT_SLUG/build/$BUILD_VERSION"",
      ""icon_url"": ""$AVATAR""
    },
    ""title"": ""$COMMIT_SUBJECT"",
    ""url"": ""$URL"",
    ""description"": ""$COMMIT_MESSAGE $COMMIT_AUTHOR"",
    ""fields"": [
      {
        ""name"": ""Commit"",
        ""value"": ""[``$($env:APPVEYOR_REPO_COMMIT.substring(0, 7))``](https://github.com/$env:APPVEYOR_REPO_NAME/commit/$env:APPVEYOR_REPO_COMMIT)"",
        ""inline"": true
      },
      {
        ""name"": ""Branch/Tag"",
        ""value"": ""[``$env:APPVEYOR_REPO_BRANCH``](https://github.com/$env:APPVEYOR_REPO_NAME/tree/$env:APPVEYOR_REPO_BRANCH)"",
        ""inline"": true
      }
    ],
    ""timestamp"": ""$TIMESTAMP""
  } ]
}"

Invoke-RestMethod -Uri "$WEBHOOK_URL" -Method "POST" -UserAgent "AppVeyor-Webhook" `
  -ContentType "application/json" -Header @{"X-Author"="k3rn31p4nic#8383"} `
  -Body $WEBHOOK_DATA

Write-Output "[Webhook]: Successfully sent the webhook."
