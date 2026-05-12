# Extract mindmap from IMX file with proper text handling
param(
    [string]$IMXFile = "C:\temp\exemple.imx",
    [string]$OutputDir = "C:\temp\mindmap_exports"
)

New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

$extractPath = Join-Path $OutputDir "imx_temp"
New-Item -ItemType Directory -Path $extractPath -Force | Out-Null

# Extract ZIP
Expand-Archive -Path $IMXFile -DestinationPath $extractPath -Force

# Load XML
$dataXml = [xml](Get-Content (Join-Path $extractPath "data.xml") -Raw)
$metaXml = [xml](Get-Content (Join-Path $extractPath "mapmeta.xml") -Raw)

# Get metadata
$title = $metaXml.MapMeta.Title.value
$author = $metaXml.MapMeta.InitialAuthor.value
$created = $metaXml.MapMeta.CreationTime.value

Write-Host "Extracting mindmap: $title" -ForegroundColor Green

# Build a map of ID to element
$elemMap = @{}
$dataXml.SelectNodes(".//*[@id]") | ForEach-Object {
    $elemMap[$_.id] = $_
}

# Function to get text from a node
function Get-Text {
    param($elem)

    if (-not $elem) { return "" }

    # Try property with key
    $prop = $elem.SelectSingleNode(".//property[@key='com.thinkbuzan.gaia.cell.text']")
    if ($prop -and $prop.value) {
        return $prop.value.Trim()
    }

    # Try HTMLLabel
    $htmlProp = $elem.SelectSingleNode(".//property[@key='com.thinkbuzan.gaia.entities.HTMLLabel']/HTMLLabel")
    if ($htmlProp -and $htmlProp.InnerText) {
        $text = $htmlProp.InnerText
        # Remove HTML tags
        $text = $text -replace '<[^>]+>', ''
        return $text.Trim()
    }

    return ""
}

# Function to get color
function Get-Color {
    param($elem)
    if ($elem.style -match "strokeColor=([#\w]+)") {
        return $matches[1]
    }
    return $null
}

# Build tree recursively
function Build-Tree {
    param($elemId, [int]$depth = 0)

    $elem = $elemMap[$elemId]
    if (-not $elem) { return $null }

    $text = Get-Text $elem
    $color = Get-Color $elem

    $node = @{
        text = $text
        id = $elemId
        depth = $depth
        children = @()
    }

    if ($color) {
        $node["color"] = $color
    }

    # Find branches from this element
    $branches = $dataXml.SelectNodes(".//branch[@source='$elemId']")

    foreach ($branch in $branches) {
        $targetId = $branch.target
        $branchText = Get-Text $branch

        $childNode = Build-Tree $targetId ($depth + 1)
        if ($childNode) {
            if ($branchText) {
                $childNode["branch"] = $branchText
            }
            $node.children += $childNode
        }
    }

    return $node
}

# Find root (central idea)
$root = $dataXml.SelectSingleNode(".//floatingIdea")
$rootText = Get-Text $root

# Build complete tree
$tree = Build-Tree $root.id

Write-Host "Root: $rootText" -ForegroundColor Cyan

# Export to JSON
$jsonData = @{
    metadata = @{
        title = $title
        author = $author
        created = $created
    }
    mindmap = $tree
}

$jsonFile = Join-Path $OutputDir "mindmap.json"
$jsonData | ConvertTo-Json -Depth 20 | Out-File $jsonFile -Encoding UTF8
Write-Host "JSON: $jsonFile" -ForegroundColor Green

# Export to simple text outline (easy to read)
function Build-Text {
    param($node, [int]$indent = 0)
    $prefix = "  " * $indent

    if ($node.branch) {
        Write-Output "$prefix- $($node.branch)"
        Write-Output "$prefix  $($node.text)"
    } else {
        Write-Output "$prefix$($node.text)"
    }

    foreach ($child in $node.children) {
        Build-Text $child ($indent + 1)
    }
}

$textFile = Join-Path $OutputDir "mindmap.txt"
$outline = @"
$title
Author: $author
Created: $created

"@
Build-Text $tree | ForEach-Object { $outline += "$_`n" }
$outline | Out-File $textFile -Encoding UTF8
Write-Host "TEXT: $textFile" -ForegroundColor Green

# Export to OPML
function Build-OPML {
    param($node)

    $text = if ($node.branch) {
        "$($node.branch): $($node.text)"
    } else {
        $node.text
    }

    $text = $text -replace '&', '&amp;' -replace '<', '&lt;' -replace '>', '&gt;' -replace '"', '&quot;'

    if ($node.children.Count -eq 0) {
        return "<outline text=`"$text`" />"
    }

    $outline = "<outline text=`"$text`">`n"
    foreach ($child in $node.children) {
        $outline += (Build-OPML $child) + "`n"
    }
    $outline += "</outline>"

    return $outline
}

$opmlFile = Join-Path $OutputDir "mindmap.opml"
$opml = @"
<?xml version="1.0" encoding="utf-8"?>
<opml version="2.0">
  <head>
    <title>$title</title>
    <ownerName>$author</ownerName>
    <dateCreated>$created</dateCreated>
  </head>
  <body>
    $(Build-OPML $tree)
  </body>
</opml>
"@
$opml | Out-File $opmlFile -Encoding UTF8
Write-Host "OPML: $opmlFile" -ForegroundColor Green

# Cleanup
Remove-Item -Path $extractPath -Recurse -Force

Write-Host "`nDone! All formats exported to: $OutputDir" -ForegroundColor Green
