param(
    [Parameter(Mandatory=$true)][string]$MarkdownPath,
    [Parameter(Mandatory=$true)][string]$TemplatePath,
    [Parameter(Mandatory=$true)][string]$HtmlPath,
    [Parameter(Mandatory=$true)][string]$PdfPath,
    [string]$Title = "PinProbe A1 文档",
    [string]$Subtitle = "",
    [string]$DocDate = "2026-07-27"
)

$ErrorActionPreference = "Stop"

function HtmlEncode([string]$Text) {
    return [System.Net.WebUtility]::HtmlEncode($Text)
}

function InlineMd([string]$Text) {
    $encoded = HtmlEncode $Text
    $encoded = [regex]::Replace($encoded, '\*\*([^*]+)\*\*', '<strong>$1</strong>')
    $encoded = [regex]::Replace($encoded, '`([^`]+)`', '<code>$1</code>')
    return $encoded
}

$template = Get-Content -LiteralPath $TemplatePath -Encoding UTF8 -Raw
$logoMatch = [regex]::Match($template, '<g id="gts-logo">.*?</g>', [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $logoMatch.Success) {
    throw "Cannot find gts-logo in template."
}
$logoSymbol = $logoMatch.Value

$lines = Get-Content -LiteralPath $MarkdownPath -Encoding UTF8
$content = [System.Text.StringBuilder]::new()
$inCode = $false
$inTable = $false
$tableRowIndex = 0
$inUl = $false
$inOl = $false

function Close-Blocks {
    param([System.Text.StringBuilder]$Builder)
    if ($script:inTable) {
        [void]$Builder.AppendLine("</tbody></table>")
        $script:inTable = $false
        $script:tableRowIndex = 0
    }
    if ($script:inUl) {
        [void]$Builder.AppendLine("</ul>")
        $script:inUl = $false
    }
    if ($script:inOl) {
        [void]$Builder.AppendLine("</ol>")
        $script:inOl = $false
    }
}

foreach ($line in $lines) {
    $raw = $line

    if ($raw -match '^```') {
        Close-Blocks $content
        if (-not $inCode) {
            [void]$content.AppendLine("<pre><code>")
            $inCode = $true
        } else {
            [void]$content.AppendLine("</code></pre>")
            $inCode = $false
        }
        continue
    }

    if ($inCode) {
        [void]$content.AppendLine((HtmlEncode $raw))
        continue
    }

    if ($raw.Trim() -eq "") {
        Close-Blocks $content
        continue
    }

    if ($raw.Trim() -eq "---") {
        Close-Blocks $content
        [void]$content.AppendLine("<hr>")
        continue
    }

    if ($raw -match '^\|(.+)\|\s*$') {
        $cells = $raw.Trim('|') -split '\|'
        $isSep = $true
        foreach ($c in $cells) {
            if ($c.Trim() -notmatch '^:?-{3,}:?$') { $isSep = $false }
        }
        if ($isSep) { continue }

        if (-not $inTable) {
            Close-Blocks $content
            [void]$content.AppendLine("<table>")
            $inTable = $true
            $tableRowIndex = 0
        }

        if ($tableRowIndex -eq 0) {
            [void]$content.Append("<thead><tr>")
            foreach ($c in $cells) {
                [void]$content.Append("<th>" + (InlineMd $c.Trim()) + "</th>")
            }
            [void]$content.AppendLine("</tr></thead><tbody>")
        } else {
            [void]$content.Append("<tr>")
            foreach ($c in $cells) {
                [void]$content.Append("<td>" + (InlineMd $c.Trim()) + "</td>")
            }
            [void]$content.AppendLine("</tr>")
        }
        $tableRowIndex++
        continue
    } elseif ($inTable) {
        Close-Blocks $content
    }

    if ($raw -match '^(#{1,6})\s+(.+)$') {
        Close-Blocks $content
        $level = [Math]::Min($Matches[1].Length, 4)
        $text = InlineMd $Matches[2]
        [void]$content.AppendLine("<h$level>$text</h$level>")
        continue
    }

    if ($raw -match '^>\s*(.+)$') {
        Close-Blocks $content
        [void]$content.AppendLine("<blockquote>" + (InlineMd $Matches[1]) + "</blockquote>")
        continue
    }

    if ($raw -match '^\d+\.\s+(.+)$') {
        if (-not $inOl) {
            if ($inUl) { [void]$content.AppendLine("</ul>"); $inUl = $false }
            [void]$content.AppendLine("<ol>")
            $inOl = $true
        }
        [void]$content.AppendLine("<li>" + (InlineMd $Matches[1]) + "</li>")
        continue
    }

    if ($raw -match '^[-*]\s+(.+)$') {
        if (-not $inUl) {
            if ($inOl) { [void]$content.AppendLine("</ol>"); $inOl = $false }
            [void]$content.AppendLine("<ul>")
            $inUl = $true
        }
        [void]$content.AppendLine("<li>" + (InlineMd $Matches[1]) + "</li>")
        continue
    }

    Close-Blocks $content
    [void]$content.AppendLine("<p>" + (InlineMd $raw) + "</p>")
}

Close-Blocks $content
if ($inCode) {
    [void]$content.AppendLine("</code></pre>")
}

$html = @"
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<title>$([System.Net.WebUtility]::HtmlEncode($Title))</title>
<style>
* { box-sizing: border-box; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
@page { size: A4 portrait; margin: 24mm 15mm 20mm; }
body {
    margin: 0;
    color: #1a1a1a;
    font-family: "Microsoft YaHei", "SimSun", "DengXian", Arial, sans-serif;
    font-size: 10.5pt;
    line-height: 1.55;
    background: #d7d9df;
    padding: 18px 0;
}
.print-header {
    width: 210mm;
    margin: 0 auto;
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 10mm 16mm 5mm;
    border-bottom: 5px solid #d9e1ef;
    background: #fff;
}
.print-header .brand { font-size: 10pt; color: #1F3864; font-weight: 700; }
.logo { height: 28px; width: auto; overflow: visible; display: block; }
.cover {
    width: 210mm;
    min-height: 278mm;
    margin: 0 auto 18px;
    padding: 6mm 16mm;
    background: #fff;
    display: flex;
    flex-direction: column;
    box-shadow: 0 3px 14px rgba(0,0,0,.16);
}
.cover-top { display: none; }
.cover-main { flex: 1; display: flex; flex-direction: column; justify-content: center; text-align: center; }
.cover .logo { height: 45px; }
.cover h1 { font-size: 28pt; line-height: 1.25; margin: 0 0 12px; font-weight: 800; color: #1a1a1a; }
.cover-divider { width: 76px; height: 4px; background: #1F3864; margin: 22px auto; }
.cover-subtitle { color: #1F3864; font-size: 13pt; margin-bottom: 44px; }
.cover-info { color: #666; font-size: 11pt; line-height: 1.9; }
.cover-footer { display: none; }
.content {
    width: 210mm;
    margin: 0 auto 18px;
    padding: 6mm 16mm;
    background: #fff;
    box-shadow: 0 3px 14px rgba(0,0,0,.16);
    page-break-before: always;
}
h1 {
    font-size: 20pt;
    margin: 0 0 12px;
    padding-bottom: 8px;
    border-bottom: 2px solid #1F3864;
}
h2 {
    font-size: 15pt;
    color: #1F3864;
    margin: 18px 0 8px;
    padding-bottom: 5px;
    border-bottom: 1.5px solid #1F3864;
    break-before: page;
    page-break-before: always;
    break-after: avoid;
}
.content > h2:first-of-type {
    break-before: auto;
    page-break-before: auto;
}
h3 {
    font-size: 12.5pt;
    color: #1F3864;
    margin: 14px 0 6px;
    break-after: avoid;
}
h4 {
    font-size: 11.5pt;
    margin: 12px 0 5px;
    color: #333;
    break-after: avoid;
}
p { margin: 6px 0; }
hr { border: 0; border-top: 1px solid #d0d6e4; margin: 12px 0; }
table {
    width: 100%;
    border-collapse: collapse;
    margin: 8px 0 14px;
    font-size: 9.4pt;
    break-inside: avoid;
    page-break-inside: avoid;
}
thead { display: table-header-group; }
tr { break-inside: avoid; page-break-inside: avoid; }
pre, blockquote, ul, ol {
    break-inside: avoid;
    page-break-inside: avoid;
}
th {
    background: #1F3864;
    color: #fff;
    font-weight: 600;
    text-align: left;
    padding: 6px 7px;
    border: 1px solid #1a2e52;
}
td {
    padding: 5px 7px;
    border: 1px solid #bcc5d3;
    vertical-align: top;
}
tbody tr:nth-child(even) td { background: #f2f6fc; }
code {
    font-family: Consolas, "Courier New", monospace;
    font-size: 9.2pt;
    background: #f5f7fa;
    border: 1px solid #e1e4ea;
    border-radius: 3px;
    padding: 0 3px;
}
th code {
    color: #fff;
    background: rgba(255,255,255,.16);
    border-color: rgba(255,255,255,.28);
    font-weight: 700;
}
pre {
    white-space: pre-wrap;
    word-break: break-word;
    background: #f5f7fa;
    border: 1px solid #d8dee8;
    padding: 8px;
    margin: 8px 0 12px;
    break-inside: avoid;
}
pre code { border: 0; padding: 0; background: transparent; }
blockquote {
    margin: 8px 0 12px;
    padding: 7px 10px;
    border-left: 4px solid #1F3864;
    background: #f4f7fb;
    color: #333;
}
ul, ol { margin: 6px 0 10px 20px; padding: 0; }
li { margin: 3px 0; }
.print-footer {
    width: 210mm;
    margin: 0 auto 18px;
    padding: 3mm 16mm 4mm;
    border-top: 5px solid #d9e1ef;
    background: #fff;
    font-size: 8pt;
    color: #999;
    display: flex;
    justify-content: space-between;
}
@media print {
    body {
        width: 210mm;
        min-width: 210mm;
        max-width: 210mm;
        background: #fff;
        padding: 0;
    }
    .print-header {
        position: fixed;
        top: 0;
        left: 15mm;
        right: 15mm;
        width: auto;
        height: 19mm;
        margin: 0;
        padding: 8mm 0 4mm;
        z-index: 10;
    }
    .print-footer {
        position: fixed;
        bottom: 0;
        left: 15mm;
        right: 15mm;
        width: auto;
        height: 15mm;
        margin: 0;
        padding: 3mm 0 4mm;
        z-index: 10;
    }
    .cover {
        width: auto;
        min-height: 245mm;
        margin: 0;
        padding: 0;
        box-shadow: none;
        break-after: page;
        page-break-after: always;
    }
    .content {
        width: auto;
        margin: 0;
        padding: 0;
        box-shadow: none;
    }
}
</style>
</head>
<body>
<svg width="0" height="0" style="position:absolute">
$logoSymbol
</svg>

<div class="print-header">
  <span class="brand">PinProbe A1 / SCPI Command Reference</span>
  <svg class="logo" viewBox="4400 6500 18750 3520" role="img" aria-label="GTS"><use href="#gts-logo"></use></svg>
</div>
<div class="print-footer">
  <span>PinProbe A1 Box Control</span>
  <span>Customer Delivery | $([System.Net.WebUtility]::HtmlEncode($DocDate))</span>
</div>

<section class="cover">
  <div class="cover-top">
    <svg class="logo" viewBox="4400 6500 18750 3520" role="img" aria-label="GTS"><use href="#gts-logo"></use></svg>
  </div>
  <div class="cover-main">
    <h1>$([System.Net.WebUtility]::HtmlEncode($Title))</h1>
    <div class="cover-divider"></div>
    <div class="cover-subtitle">$([System.Net.WebUtility]::HtmlEncode($Subtitle))</div>
    <div class="cover-info">Customer Delivery<br>$([System.Net.WebUtility]::HtmlEncode($DocDate))</div>
  </div>
  <div class="cover-footer">
    <span>PinProbe A1</span>
    <span>GTS</span>
  </div>
</section>

<main class="content">
$content
</main>
</body>
</html>
"@

$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($HtmlPath, $html, $utf8NoBom)

$edgeCandidates = @(
    "C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    "C:\Program Files\Microsoft\Edge\Application\msedge.exe"
)
$edge = $edgeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $edge) {
    throw "Microsoft Edge was not found."
}

$tempProfile = Join-Path ([System.IO.Path]::GetTempPath()) ("edge-pdf-" + [System.Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempProfile | Out-Null

$htmlUri = ([System.Uri](Resolve-Path -LiteralPath $HtmlPath).Path).AbsoluteUri
$pdfFull = [System.IO.Path]::GetFullPath($PdfPath)
$pdfDir = [System.IO.Path]::GetDirectoryName($pdfFull)
if (-not (Test-Path -LiteralPath $pdfDir)) {
    New-Item -ItemType Directory -Path $pdfDir | Out-Null
}
if (Test-Path -LiteralPath $pdfFull) {
    Remove-Item -LiteralPath $pdfFull -Force
}

$tempPdf = Join-Path $tempProfile "out.pdf"
$edgeLog = Join-Path $tempProfile "edge-stderr.log"

function Quote-ProcessArg([string]$Value) {
    '"' + ($Value -replace '"', '\"') + '"'
}

$psi = [System.Diagnostics.ProcessStartInfo]::new()
$psi.FileName = $edge
$psi.UseShellExecute = $false
$psi.CreateNoWindow = $true
$psi.RedirectStandardError = $true
$psi.RedirectStandardOutput = $true
$edgeArgs = @(
    "--headless=new",
    "--disable-gpu",
    "--disable-extensions",
    "--no-first-run",
    "--user-data-dir=$tempProfile",
    "--no-pdf-header-footer",
    "--print-to-pdf=$tempPdf",
    $htmlUri
)
$psi.Arguments = (($edgeArgs | ForEach-Object { Quote-ProcessArg $_ }) -join " ")

$process = [System.Diagnostics.Process]::Start($psi)
$stderrTask = $process.StandardError.ReadToEndAsync()
$stdoutTask = $process.StandardOutput.ReadToEndAsync()
if (-not $process.WaitForExit(60000)) {
    try { $process.Kill() } catch {}
    throw "Edge PDF export timed out."
}
$stderr = $stderrTask.Result
$stdout = $stdoutTask.Result
if ($stderr) { [System.IO.File]::WriteAllText($edgeLog, $stderr, [System.Text.UTF8Encoding]::new($false)) }

Start-Sleep -Milliseconds 1200
if (-not (Test-Path -LiteralPath $tempPdf)) {
    $detail = if ($stderr) { $stderr } elseif ($stdout) { $stdout } else { "No Edge output." }
    throw "PDF was not created: $pdfFull`n$detail"
}

Move-Item -LiteralPath $tempPdf -Destination $pdfFull -Force
Get-Item -LiteralPath $pdfFull | Select-Object FullName,Length,LastWriteTime
