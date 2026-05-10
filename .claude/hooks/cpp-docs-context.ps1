$j = [Console]::In.ReadToEnd() | ConvertFrom-Json
$fp = $j.tool_input.file_path
if ($fp -match '\.(cpp|hpp|h)$') {
    $docsRoot = 'C:\Users\Neilwinn Pineda\Documents\Engine-Chili2.0\docs'
    $archi = Get-Content "$docsRoot\engine\ARCHI_RULES" -Raw -ErrorAction SilentlyContinue
    $readme = Get-Content "$docsRoot\README.md" -Raw -ErrorAction SilentlyContinue
    if ($archi -or $readme) {
        $ctx = "=== ARCHI_RULES (must read before editing C++) ===`n$archi`n`n=== docs/README.md ===`n$readme"
        @{
            hookSpecificOutput = @{
                hookEventName = "PreToolUse"
                additionalContext = $ctx
            }
        } | ConvertTo-Json -Depth 5
    }
}
