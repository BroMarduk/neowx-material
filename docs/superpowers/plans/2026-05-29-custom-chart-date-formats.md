# Custom Chart Date Formats Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let custom charts (`customChart*`) override their x-axis label and/or tooltip date format using named formats defined in `skin.conf` `[[Formatting]]`, at chart level and per-page override level.

**Architecture:** A new standalone JS helper `neowxDateFormatter(fmt)` (its own file, loaded before `app.js`) centralizes moment.js formatting and the existing `:MM`→`:mm` fix. The seven chart-rendering Cheetah templates resolve two new optional keys (`datetime_label_format`, `datetime_tooltip_format`) — page override → chart top-level → unset — look the named key up in `[[Formatting]]`, and, when set, inject `xaxis`/`tooltip` overrides into the ApexCharts options after the shared-config spread, calling the helper. Unset/unknown keys fall back to the page's existing default formatter (no override emitted). The helper is the reusable foundation for the future per-page and built-in-chart tiers noted in the spec.

**Tech Stack:** Cheetah templates (WeeWX skin), ApexCharts, moment.js (vendored), Node.js (for the helper unit test, no test framework — plain `assert`).

**Spec:** `docs/superpowers/specs/2026-05-29-custom-chart-date-formats-design.md`

---

## File Structure

**Create:**
- `skins/neowx-material/js/neowx-date-format.js` — the `neowxDateFormatter` helper. One responsibility: turn a moment format string into a formatter function. Browser global + Node `module.exports`.
- `tests/neowx-date-format.test.js` — Node `assert` test for the helper (repo root; not deployed by `CopyGenerator`).

**Modify:**
- `skins/neowx-material/js.inc` — add a `<script>` tag for the new helper (after moment loads, before `app.js`).
- The seven chart-rendering templates (custom-chart loop only):
  - `skins/neowx-material/index.html.tmpl`
  - `skins/neowx-material/yesterday.html.tmpl`
  - `skins/neowx-material/week.html.tmpl`
  - `skins/neowx-material/month.html.tmpl`
  - `skins/neowx-material/month-%Y-%m.html.tmpl`
  - `skins/neowx-material/year.html.tmpl`
  - `skins/neowx-material/year-%Y.html.tmpl`
- `skins/neowx-material/skin.conf` — document the new keys, add example `datetime_custom_*` formats, and a usage example on `customChartOutTemp`.

**Note on Cheetah indentation:** Leading whitespace before `#`-directives (`#set`, `#if`, `#end if`) is stripped by Cheetah and is purely cosmetic. Indentation on emitted JS lines is also cosmetic (it's inside a `<script>`). Match the surrounding indentation of each file for readability, but it does not affect behavior.

---

## Task 1: Create the `neowxDateFormatter` helper (TDD)

**Files:**
- Create: `skins/neowx-material/js/neowx-date-format.js`
- Test: `tests/neowx-date-format.test.js`

- [ ] **Step 1: Write the failing test**

Create `tests/neowx-date-format.test.js`:

```js
// Unit test for the neowxDateFormatter chart-date helper.
// Run: node tests/neowx-date-format.test.js
const assert = require('assert');
const path = require('path');

// Load the exact vendored moment build the skin ships and expose it as the
// global the helper reads (mirrors the browser, where moment is a global).
global.moment = require(
    path.join(__dirname, '..', 'skins', 'neowx-material', 'js', 'vendor', 'moment.min.js')
);

const neowxDateFormatter = require(
    path.join(__dirname, '..', 'skins', 'neowx-material', 'js', 'neowx-date-format.js')
);

// Arbitrary fixed unix timestamp. Assertions compare the helper against a direct
// moment call with the SAME timestamp, so the absolute value is irrelevant and
// the test is timezone-independent.
const TS = 1767322445;

// 1. Axis-style call: ApexCharts passes (value, timestamp); format the timestamp.
assert.strictEqual(
    neowxDateFormatter('DD.MM.YYYY')(null, TS),
    moment.unix(TS).format('DD.MM.YYYY'),
    'axis-style call should format the timestamp arg'
);

// 2. Tooltip-style call: ApexCharts passes only (value); format that value.
assert.strictEqual(
    neowxDateFormatter('DD.MM.YYYY')(TS),
    moment.unix(TS).format('DD.MM.YYYY'),
    'tooltip-style call should fall back to the first arg'
);

// 3. The ":MM" minutes mistake is converted to moment's ":mm".
const out = neowxDateFormatter('HH:MM')(null, TS);
assert.strictEqual(out, moment.unix(TS).format('HH:mm'), '":MM" should become ":mm"');
assert.ok(/^\d{2}:\d{2}$/.test(out), 'expected HH:mm shape, got ' + out);

// 4. Month token "MM" (not preceded by ":") is left untouched.
assert.strictEqual(
    neowxDateFormatter('DD.MM')(null, TS),
    moment.unix(TS).format('DD.MM'),
    'month MM must not be turned into minutes'
);

console.log('neowx-date-format: all tests passed');
```

- [ ] **Step 2: Run test to verify it fails**

Run: `node tests/neowx-date-format.test.js`
Expected: FAIL — `Cannot find module '.../skins/neowx-material/js/neowx-date-format.js'`

- [ ] **Step 3: Write minimal implementation**

Create `skins/neowx-material/js/neowx-date-format.js`:

```js
// neowx-date-format.js
// Returns a moment.js formatter for the given format string.
//
// Centralizes the ":MM" -> ":mm" minutes-token fix that the chart configs need:
// users commonly write uppercase MM for minutes, but moment uses lowercase mm
// (uppercase MM is the month). We only rewrite ":MM" so month tokens are safe.
//
// The returned function accepts ApexCharts' two calling conventions:
//   - axis labels:  formatter(value, timestamp)  -> formats `timestamp`
//   - tooltip x:    formatter(value)             -> formats `value`
function neowxDateFormatter(fmt) {
    fmt = String(fmt).replace(/:MM/g, ':mm');
    return function (val, timestamp) {
        var ts = (typeof timestamp !== 'undefined') ? timestamp : val;
        return moment.unix(ts).format(fmt);
    };
}

// Export for Node-based unit tests. Ignored in the browser, where this file is
// loaded as a plain script and `neowxDateFormatter` becomes a global.
if (typeof module !== 'undefined' && module.exports) {
    module.exports = neowxDateFormatter;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `node tests/neowx-date-format.test.js`
Expected: PASS — prints `neowx-date-format: all tests passed`, exit code 0

- [ ] **Step 5: Commit**

```bash
git add tests/neowx-date-format.test.js skins/neowx-material/js/neowx-date-format.js
git commit -m "Add neowxDateFormatter chart-date helper with node test"
```

---

## Task 2: Load the helper in `js.inc`

**Files:**
- Modify: `skins/neowx-material/js.inc:20-22`

The helper must be loaded before `app.js` and after moment (so the global `moment` exists when charts render). The moment `<script>` block ends at line 19; `apexcharts` loads at line 20; `app.js` at line 22.

- [ ] **Step 1: Add the script tag**

Edit `skins/neowx-material/js.inc`. Find:

```
<script src="js/vendor/apexcharts/apexcharts.min.js"></script>
## NeoWX Material App
<script type="text/javascript" src="js/app.js"></script>
```

Replace with:

```
<script src="js/vendor/apexcharts/apexcharts.min.js"></script>
## NeoWX Material chart helpers (shared date formatter)
<script type="text/javascript" src="js/neowx-date-format.js"></script>
## NeoWX Material App
<script type="text/javascript" src="js/app.js"></script>
```

- [ ] **Step 2: Verify the include is present and ordered correctly**

Run: `grep -n "neowx-date-format.js\|app.js\|apexcharts.min.js" skins/neowx-material/js.inc`
Expected: `apexcharts.min.js` line < `neowx-date-format.js` line < `app.js` line.

- [ ] **Step 3: Commit**

```bash
git add skins/neowx-material/js.inc
git commit -m "Load neowx-date-format helper before app.js in js.inc"
```

---

## Task 3: Apply override logic to all seven chart templates

Each of the seven templates gets the **same three edits** inside its custom-chart loop. Treat one file at a time; the three edits in a file are one logical change (a half-edited file emits broken JS), so apply all three before moving on. Commit once after all seven files are done.

**Files (custom-chart loop in each):**
- `skins/neowx-material/index.html.tmpl`
- `skins/neowx-material/yesterday.html.tmpl`
- `skins/neowx-material/week.html.tmpl`
- `skins/neowx-material/month.html.tmpl`
- `skins/neowx-material/month-%Y-%m.html.tmpl`
- `skins/neowx-material/year.html.tmpl`
- `skins/neowx-material/year-%Y.html.tmpl`

### Edit A — reserve the two new keys

The literal `['column', 'values', 'title', 'charttype']` appears **exactly twice** in each file (the `#set $CC_RESERVED` line and the `#set $cc_field_map = dict(...)` comprehension). Both must include the new keys so they are not mistaken for per-field column assignments. Because the literal is identical in both spots, replace **all occurrences** in the file:

Find (all occurrences):
```
['column', 'values', 'title', 'charttype']
```
Replace with:
```
['column', 'values', 'title', 'charttype', 'datetime_label_format', 'datetime_tooltip_format']
```

### Edit B — resolve the formats

Immediately **after** the `#set $cc_field_map = dict(...)` line (and before the render `#if`), insert this block (match the file's indentation):

```
                        ## Resolve custom date-format overrides (page subsection → chart top-level).
                        ## Value is a named key defined in [[Formatting]]; empty/unknown → no override
                        ## (the page's default formatter from graph_*_config stands).
                        #set $cc_label_key   = $cc_page.get('datetime_label_format',   $cc.get('datetime_label_format',   ''))
                        #set $cc_tooltip_key = $cc_page.get('datetime_tooltip_format', $cc.get('datetime_tooltip_format', ''))
                        #set $cc_label_raw   = $Extras.Formatting.get($cc_label_key,   '') if $cc_label_key   else ''
                        #set $cc_tooltip_raw = $Extras.Formatting.get($cc_tooltip_key, '') if $cc_tooltip_key else ''
                        ## ConfigObj parses a value containing a comma as a list; rejoin to a moment string.
                        #set $cc_label_fmt   = ', '.join($cc_label_raw)   if isinstance($cc_label_raw,   list) else $cc_label_raw
                        #set $cc_tooltip_fmt = ', '.join($cc_tooltip_raw) if isinstance($cc_tooltip_raw, list) else $cc_tooltip_raw
```

### Edit C — inject the overrides into the ApexCharts options

Immediately **after** the `...graph_${cc_type}_config,` line (and before the existing `yaxis: {` line), insert this block (match the file's indentation):

```
                                    #if $cc_label_fmt
                                    xaxis: {
                                        ...graph_${cc_type}_config.xaxis,
                                        labels: {
                                            ...graph_${cc_type}_config.xaxis.labels,
                                            formatter: neowxDateFormatter("$cc_label_fmt")
                                        }
                                    },
                                    #end if
                                    #if $cc_tooltip_fmt
                                    tooltip: {
                                        ...graph_${cc_type}_config.tooltip,
                                        x: {
                                            ...graph_${cc_type}_config.tooltip.x,
                                            formatter: neowxDateFormatter("$cc_tooltip_fmt")
                                        }
                                    },
                                    #end if
```

These blocks add new keys (`xaxis`, `tooltip`) that the spread does not collide with (`yaxis`, `series` are separate); object-literal key order means the explicit `xaxis`/`tooltip` override the spread's. The `xaxis.labels` / `tooltip.x` sub-spreads preserve `type: 'datetime'`, `tickAmount`, etc. from the base config, replacing only the formatter.

- [ ] **Step 1: `index.html.tmpl`** — apply Edit A (replace-all), Edit B (after `#set $cc_field_map` ~line 773), Edit C (after `...graph_${cc_type}_config,` ~line 801).
- [ ] **Step 2: `yesterday.html.tmpl`** — Edit A; Edit B (after ~line 650); Edit C (after ~line 677).
- [ ] **Step 3: `week.html.tmpl`** — Edit A; Edit B (after ~line 599); Edit C (after ~line 630).
- [ ] **Step 4: `month.html.tmpl`** — Edit A; Edit B (after ~line 656); Edit C (after ~line 683).
- [ ] **Step 5: `month-%Y-%m.html.tmpl`** — Edit A; Edit B (after ~line 527); Edit C (after ~line 554).
- [ ] **Step 6: `year.html.tmpl`** — Edit A; Edit B (after ~line 516); Edit C (after ~line 547).
- [ ] **Step 7: `year-%Y.html.tmpl`** — Edit A; Edit B (after ~line 555); Edit C (after ~line 582).

- [ ] **Step 8: Verify all seven files got all three edits**

Run:
```bash
grep -c "datetime_label_format', 'datetime_tooltip_format" skins/neowx-material/index.html.tmpl skins/neowx-material/yesterday.html.tmpl skins/neowx-material/week.html.tmpl skins/neowx-material/month.html.tmpl "skins/neowx-material/month-%Y-%m.html.tmpl" skins/neowx-material/year.html.tmpl "skins/neowx-material/year-%Y.html.tmpl"
```
Expected: each file reports `2` (Edit A hit both the CC_RESERVED line and the comprehension).

Run:
```bash
grep -c "neowxDateFormatter(" skins/neowx-material/index.html.tmpl skins/neowx-material/yesterday.html.tmpl skins/neowx-material/week.html.tmpl skins/neowx-material/month.html.tmpl "skins/neowx-material/month-%Y-%m.html.tmpl" skins/neowx-material/year.html.tmpl "skins/neowx-material/year-%Y.html.tmpl"
```
Expected: each file reports `2` (label + tooltip injection from Edit C).

Run:
```bash
grep -c "cc_label_fmt   = ', '.join" skins/neowx-material/index.html.tmpl skins/neowx-material/yesterday.html.tmpl skins/neowx-material/week.html.tmpl skins/neowx-material/month.html.tmpl "skins/neowx-material/month-%Y-%m.html.tmpl" skins/neowx-material/year.html.tmpl "skins/neowx-material/year-%Y.html.tmpl"
```
Expected: each file reports `1` (Edit B present).

- [ ] **Step 9: Commit**

```bash
git add skins/neowx-material/index.html.tmpl skins/neowx-material/yesterday.html.tmpl skins/neowx-material/week.html.tmpl skins/neowx-material/month.html.tmpl "skins/neowx-material/month-%Y-%m.html.tmpl" skins/neowx-material/year.html.tmpl "skins/neowx-material/year-%Y.html.tmpl"
git commit -m "Apply per-chart date-format overrides to all chart templates"
```

---

## Task 4: Document and exemplify in `skin.conf`

**Files:**
- Modify: `skins/neowx-material/skin.conf:243-253` (the `[[Formatting]]` block) and `skins/neowx-material/skin.conf:358-371` (`customChartOutTemp` example).

- [ ] **Step 1: Add named-format docs + examples to `[[Formatting]]`**

Edit `skins/neowx-material/skin.conf`. Find:

```
        # Datetime format (javascript) for charts
        # DD = day of month (01..31)
        # MM = month (01..12)
        # HH = 24-hour hour (00..23)
        # mm = minutes (00..59) <-- lowercase mm is required
        # dd / ddd / dddd = weekday (different lengths for different names)
        datetime_graph_label = dd DD HH:mm
        datetime_graph_tooltip = dd DD.MM. HH:mm
        datetime_graph_archive = DD.MM.YY
```

Replace with:

```
        # Datetime format (javascript) for charts
        # DD = day of month (01..31)
        # MM = month (01..12)
        # HH = 24-hour hour (00..23)
        # mm = minutes (00..59) <-- lowercase mm is required
        # dd / ddd / dddd = weekday (different lengths for different names)
        datetime_graph_label = dd DD HH:mm
        datetime_graph_tooltip = dd DD.MM. HH:mm
        datetime_graph_archive = DD.MM.YY

        # Custom named chart date formats (javascript / moment.js tokens)
        # ---------------------------------------------------------------------
        # Define any number of named formats here, by convention prefixed with
        # "datetime_custom_". A custom chart can then reference one by name to
        # override its x-axis labels and/or tooltip date (see the customChart*
        # examples in the [[Appearance]] section):
        #
        #   datetime_label_format   = datetime_custom_dayonly   # x-axis labels
        #   datetime_tooltip_format = datetime_custom_full       # hover tooltip
        #
        # These keys work at chart level and inside a per-page override
        # subsection ([[[[current]]]], [[[[yesterday]]]], [[[[week]]]],
        # [[[[month]]]], [[[[year]]]]). An unset or unknown name falls back to
        # the chart's normal format. Avoid commas inside a format value.
        #
        # datetime_custom_dayonly = ddd DD
        # datetime_custom_full    = ddd DD.MM.YYYY HH:mm
```

- [ ] **Step 2: Add a usage example to `customChartOutTemp`**

Edit `skins/neowx-material/skin.conf`. Find:

```
        [[[customChartOutTemp]]]
            title = Outdoor Temperature
            charttype = area
            column = avg
            values = outTemp, dewpoint
            [[[[week]]]]
                outTemp  = min, max
                dewpoint = avg
```

Replace with:

```
        [[[customChartOutTemp]]]
            title = Outdoor Temperature
            charttype = area
            column = avg
            values = outTemp, dewpoint
            # Optional: override this chart's date formats with a name from
            # [[Formatting]] above. Uncomment after defining the custom formats.
            # datetime_label_format   = datetime_custom_dayonly
            # datetime_tooltip_format = datetime_custom_full
            [[[[week]]]]
                outTemp  = min, max
                dewpoint = avg
                # Per-page override example (week page only):
                # datetime_label_format = datetime_custom_full
```

- [ ] **Step 3: Verify config still parses**

Run: `python skins/neowx-material/config_beautifier.py --help` is not required; instead confirm there are no tab/structure errors by a round-trip parse:
```bash
python -c "import configobj; configobj.ConfigObj('skins/neowx-material/skin.conf', file_error=True); print('skin.conf parses OK')"
```
Expected: `skin.conf parses OK` (if `configobj` is unavailable, skip — it ships with WeeWX at runtime).

- [ ] **Step 4: Commit**

```bash
git add skins/neowx-material/skin.conf
git commit -m "Document custom chart date-format keys with examples in skin.conf"
```

---

## Task 5: Integrated verification (manual render)

No automated harness renders the Cheetah templates; this task is the behavioral acceptance check. The helper already has unit coverage (Task 1) and the template edits have presence coverage (Task 3, Step 8).

- [ ] **Step 1: Configure a real override**

In a WeeWX install using this skin, set in `skin.conf` `[[Formatting]]`:
```
datetime_custom_dayonly = ddd DD
datetime_custom_full    = ddd DD.MM.YYYY HH:mm
```
and on `customChartOutTemp`:
```
datetime_label_format   = datetime_custom_dayonly
datetime_tooltip_format = datetime_custom_full
[[[[week]]]]
    datetime_label_format = datetime_custom_full
```
Ensure `customChartOutTemp` is in `charts_order`.

- [ ] **Step 2: Regenerate the report**

Run WeeWX report generation (e.g. `weectl report run` / `wee_reports`, per the install).
Expected: completes without a Cheetah error on any of the seven templates.

- [ ] **Step 3: Confirm emitted JS per page**

Run (adjust `HTML_ROOT` to the install's output dir):
```bash
grep -o 'neowxDateFormatter("[^"]*")' HTML_ROOT/index.html
grep -o 'neowxDateFormatter("[^"]*")' HTML_ROOT/week.html
```
Expected on `index.html`: `neowxDateFormatter("ddd DD")` (label) and `neowxDateFormatter("ddd DD.MM.YYYY HH:mm")` (tooltip).
Expected on `week.html`: label is `neowxDateFormatter("ddd DD.MM.YYYY HH:mm")` (per-page override), tooltip is `neowxDateFormatter("ddd DD.MM.YYYY HH:mm")`.

- [ ] **Step 4: Browser check**

Open `index.html` and `week.html`. Confirm the Outdoor Temperature chart's x-axis labels and hover tooltip use the configured formats, and that **other** charts (no override keys) are visually unchanged. Open the browser console: no errors.

- [ ] **Step 5: Negative case**

Set `datetime_label_format = datetime_custom_does_not_exist` on the chart, regenerate, reload.
Expected: chart falls back to the default x-axis format; no `xaxis`/`neowxDateFormatter` override is emitted for the label on that chart; no console error. Revert afterward.

---

## Self-Review

**Spec coverage:**
- Named formats in `[[Formatting]]` with `datetime_custom_` convention → Task 4 Step 1. ✓
- `datetime_label_format` / `datetime_tooltip_format`, chart-level + per-page → Task 3 Edits B/C; Task 4 Step 2. ✓
- Named-keys-only resolution → Task 3 Edit B (`$Extras.Formatting.get(key)`). ✓
- Precedence page → top-level → default (no override) → Task 3 Edit B (`cc_page.get(..., cc.get(..., ''))`) + Edit C (`#if`). ✓
- Separate axis vs tooltip → two independent keys/blocks. ✓
- Shared `neowxDateFormatter` helper (option B) → Task 1 + Task 2. ✓
- All custom-chart types/pages merge cleanly (area/bar/line + archive) → Edit C sub-spreads; verified base configs expose `xaxis.labels` and `tooltip.x`. ✓
- Five supported scopes documented → Task 4 Step 1 comment. ✓
- Unknown/blank key falls back silently → Task 3 Edit B/C + Task 5 Step 5. ✓
- Verification (manual render + grep + negative case) → Task 5. ✓
- Out of scope (value cards, built-in charts, per-page tier, Python strftime) → untouched. ✓

**Placeholder scan:** No TBD/TODO; every code/edit step shows full content and exact commands. ✓

**Type/name consistency:** `neowxDateFormatter` (file, export, test, all 7 injections) consistent. Cheetah vars `cc_label_key`/`cc_tooltip_key`/`cc_label_raw`/`cc_tooltip_raw`/`cc_label_fmt`/`cc_tooltip_fmt` defined in Edit B and consumed in Edit C consistently. Reserved-key literal identical in Edit A and the verify grep. ✓

---

# Phase 2 — Archive scopes (`month-archive` / `year-archive`)

**Goal:** Let the two per-period archive templates carry an independent whole per-page block
(`column` / `values` / per-field maps / date formats) that inherits from the base `month` / `year`
subsection.

**Architecture:** In each archive template, build `cc_page` as `dict(base subsection)` overlaid by
`.update(archive subsection)`. Every downstream read already goes through `cc_page.get(...)`, so this
one change gives the archive pages independent config with no other edits. The new format keys are
already reserved (Phase 1, Task 3).

## Task 6: Merge the archive subsection into `cc_page` (2 templates)

**Files:**
- Modify: `skins/neowx-material/month-%Y-%m.html.tmpl` (the `#set $cc_page = $cc.get('month', {})` line, ~506)
- Modify: `skins/neowx-material/year-%Y.html.tmpl` (the `#set $cc_page = $cc.get('year', {})` line, ~534)

- [ ] **Step 1: Edit `month-%Y-%m.html.tmpl`**

Find:
```
                        #set $cc_page = $cc.get('month', {})
```
Replace with:
```
                        ## Archive page: base 'month' subsection overlaid by 'month-archive'
                        ## (archive keys win; keys absent from archive are inherited from month).
                        #set $cc_page = dict($cc.get('month', {}))
                        #silent $cc_page.update($cc.get('month-archive', {}))
```

- [ ] **Step 2: Edit `year-%Y.html.tmpl`** (note this file's loop uses shallower indentation — match it)

Find:
```
                #set $cc_page = $cc.get('year', {})
```
Replace with:
```
                ## Archive page: base 'year' subsection overlaid by 'year-archive'
                ## (archive keys win; keys absent from archive are inherited from year).
                #set $cc_page = dict($cc.get('year', {}))
                #silent $cc_page.update($cc.get('year-archive', {}))
```

- [ ] **Step 3: Verify**

```bash
grep -n "cc_page = dict(\$cc.get('month'" "skins/neowx-material/month-%Y-%m.html.tmpl"
grep -n "cc_page = dict(\$cc.get('year'"  "skins/neowx-material/year-%Y.html.tmpl"
grep -c "cc_page.update" "skins/neowx-material/month-%Y-%m.html.tmpl" "skins/neowx-material/year-%Y.html.tmpl"
grep -n "cc_page = \$cc.get('month', {})\|cc_page = \$cc.get('year', {})" "skins/neowx-material/month-%Y-%m.html.tmpl" "skins/neowx-material/year-%Y.html.tmpl"
```
Expected: the first two greps each find the new `dict(...)` line; `cc_page.update` count is `1` per file; the last grep (old plain assignment) returns **nothing** (it was replaced).

- [ ] **Step 4: Commit**

```bash
git add "skins/neowx-material/month-%Y-%m.html.tmpl" "skins/neowx-material/year-%Y.html.tmpl"
git commit -m "Add month-archive / year-archive per-page override scopes"
```
End the body with a blank line then:
`Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>`

---

# Phase 3 — Per-page tier (`GraphPageFormats` → all charts)

**Goal:** A `[[[GraphPageFormats]]]` block sets the x-axis label and/or tooltip date format for every
chart (built-in + custom) on a page, for all 7 page scopes, with archive→base inheritance; per-chart
overrides still win.

**Architecture:** Each chart template declares `#set $neowx_page_scope = '<scope>'` before
`#include "js.inc"`. `js.inc` resolves the page's effective label/tooltip once and emits
`window.NEOWX_CHART_FORMAT = { label, tooltip }`. The base `graph_*_config.inc` formatters change from
the baked `$Extras.Formatting.datetime_graph_*` literal to `neowxDateFormatter(NEOWX_CHART_FORMAT.X)`.
Because every chart spreads `...graph_${type}_config`, all charts pick up the page format. With no
`GraphPageFormats` configured, the resolution yields the existing global defaults → no visual change.

**Task order matters:** Task 7 (helper fix) → Task 8 (js.inc defines the global) → Task 9 (configs
read it) → Task 10 (templates declare scope) → Task 11 (docs). Doing 9 before 8 would reference an
undefined global.

## Task 7: Fix `neowxDateFormatter` argument handling (+ regression test)

**Why:** ApexCharts calls **axis-label** formatters as `(value, timestampNumber)` but **tooltip**
formatters as `(value, optsObject)`. The Phase 1 helper used `typeof timestamp !== 'undefined'`, so
for tooltips it grabs the **opts object** → `moment.unix(object)` → "Invalid date". Phase 3 routes all
tooltips through the helper, so this must be correct. Fix: use the 2nd arg only when it is a number.

**Files:**
- Modify: `tests/neowx-date-format.test.js`
- Modify: `skins/neowx-material/js/neowx-date-format.js`

- [ ] **Step 1: Add the failing regression test.** In `tests/neowx-date-format.test.js`, insert
  immediately before the final `console.log(...)` line:

```js
// 5. Tooltip-style call where ApexCharts passes an options OBJECT as the 2nd arg.
//    Must format the first arg (the timestamp), NOT moment.unix(object) -> "Invalid date".
const tipOut = neowxDateFormatter('DD.MM.YYYY')(TS, { series: [], seriesIndex: 0, w: {} });
assert.strictEqual(
    tipOut,
    moment.unix(TS).format('DD.MM.YYYY'),
    'tooltip call with an opts object as 2nd arg must format the first arg'
);
```

- [ ] **Step 2: Run → fails**

Run: `node tests/neowx-date-format.test.js`
Expected: FAIL on assertion 5 (`tipOut` is `"Invalid date"`).

- [ ] **Step 3: Fix the helper.** In `skins/neowx-material/js/neowx-date-format.js`, replace:

```js
        var ts = (typeof timestamp !== 'undefined') ? timestamp : val;
```
with:
```js
        // Axis-label formatters are called (value, timestampNumber); tooltip formatters are
        // called (value, optsObject). Use the 2nd arg only when it is the numeric timestamp,
        // otherwise fall back to the first arg.
        var ts = (typeof timestamp === 'number') ? timestamp : val;
```

- [ ] **Step 4: Run → passes**

Run: `node tests/neowx-date-format.test.js`
Expected: PASS — `neowx-date-format: all tests passed`, exit 0.

- [ ] **Step 5: Commit**

```bash
git add skins/neowx-material/js/neowx-date-format.js tests/neowx-date-format.test.js
git commit -m "Fix neowxDateFormatter: use 2nd arg only when numeric (tooltip opts object)"
```
End the body with a blank line then the `Co-Authored-By` trailer.

## Task 8: Resolve + emit `NEOWX_CHART_FORMAT` in `js.inc`

**Files:**
- Modify: `skins/neowx-material/js.inc` (inside the global apexcharts `<script>` block, after the
  `window.Apex = { ... }` object literal closes and before the `## Ordinals conversion` comment /
  `getOrdinalDirection` function — around line 111).

- [ ] **Step 1: Insert the resolution + emission.** After the line that closes the `window.Apex`
  object (`    }` followed by a blank line, right before `    ## Ordinals conversion for W0CHP's...`),
  insert:

```
    ## --- Per-page chart date format (read by graph_*_config.inc formatters) ---
    #set $gpf = $Extras.Formatting.get('GraphPageFormats', {})
    #set $scope = $getVar('neowx_page_scope', '')
    #set $base_scope = 'month' if $scope == 'month-archive' else ('year' if $scope == 'year-archive' else $scope)
    #set $is_archive = $scope in ('month-archive', 'year-archive')
    #set $lbl_key = $gpf.get($scope, {}).get('label',   $gpf.get($base_scope, {}).get('label',   '')) if $scope else ''
    #set $tip_key = $gpf.get($scope, {}).get('tooltip', $gpf.get($base_scope, {}).get('tooltip', '')) if $scope else ''
    #set $lbl_fmt = $Extras.Formatting.get($lbl_key, '') if $lbl_key else ''
    #set $tip_fmt = $Extras.Formatting.get($tip_key, '') if $tip_key else ''
    #if isinstance($lbl_fmt, list)
        #set $lbl_fmt = ', '.join($lbl_fmt)
    #end if
    #if isinstance($tip_fmt, list)
        #set $tip_fmt = ', '.join($tip_fmt)
    #end if
    #if not $lbl_fmt
        #set $lbl_fmt = $Extras.Formatting.datetime_graph_archive if $is_archive else $Extras.Formatting.datetime_graph_label
    #end if
    #if not $tip_fmt
        #set $tip_fmt = $Extras.Formatting.datetime_graph_tooltip
    #end if
    window.NEOWX_CHART_FORMAT = { label: "$lbl_fmt", tooltip: "$tip_fmt" };
```

This runs for every page that includes `js.inc`. Pages that don't set `$neowx_page_scope` (telemetry,
almanac, archive, history) get `$scope = ''`, so `lbl_key`/`tip_key` are `''` and the global defaults
apply — identical to today.

- [ ] **Step 2: Verify presence**

```bash
grep -n "NEOWX_CHART_FORMAT\|neowx_page_scope\|GraphPageFormats" skins/neowx-material/js.inc
```
Expected: the resolution block plus the `window.NEOWX_CHART_FORMAT = {...}` line are present.

- [ ] **Step 3: Commit**

```bash
git add skins/neowx-material/js.inc
git commit -m "Resolve and emit per-page NEOWX_CHART_FORMAT in js.inc"
```
End the body with the `Co-Authored-By` trailer.

## Task 9: Base config formatters read `NEOWX_CHART_FORMAT` (7 `.inc` files)

**Files (each formatter; read each to confirm before replacing):**
- `skins/neowx-material/graph_area_config.inc` — label ~25, tooltip ~37
- `skins/neowx-material/graph_line_config.inc` — label ~55, tooltip ~81
- `skins/neowx-material/graph_bar_config.inc` — label ~18, tooltip ~27
- `skins/neowx-material/graph_area_archive_config.inc` — archive label ~25, tooltip ~34
- `skins/neowx-material/graph_line_archive_config.inc` — archive label ~77, tooltip ~103
- `skins/neowx-material/graph_bar_archive_config.inc` — archive label ~18, tooltip ~27
- `skins/neowx-material/graph_radar_config.inc` — tooltip ~27 (no axis formatter)

**Transformation rule.** Each formatter currently has one of these shapes (axis uses `timestamp`,
tooltip uses `val`, radar tooltip uses a destructured 2nd arg; the area label one also has an inline
comment):
```js
formatter: function(val, timestamp) {
    var fmt = "$Extras.Formatting.datetime_graph_label";   // or _archive, or _tooltip
    fmt = fmt.replace(/:MM/g, ':mm');
    return moment.unix(timestamp).format(fmt);             // or moment.unix(val)
}
```
Replace the **entire** `formatter: function(...) { ... }` (including any inline comment lines) with a
one-liner, mapping by the `fmt` literal:
- `datetime_graph_label` **or** `datetime_graph_archive` → `formatter: neowxDateFormatter(NEOWX_CHART_FORMAT.label)`
- `datetime_graph_tooltip` → `formatter: neowxDateFormatter(NEOWX_CHART_FORMAT.tooltip)`

Example — `graph_area_config.inc` becomes:
```js
    labels: {
        formatter: neowxDateFormatter(NEOWX_CHART_FORMAT.label)
    }
...
    x: {
        formatter: neowxDateFormatter(NEOWX_CHART_FORMAT.tooltip)
    }
```

There are 13 formatter blocks total (6 label/archive-label + 7 tooltip). Do one file at a time.

- [ ] **Step 1:** `graph_area_config.inc` — replace label formatter → `.label`, tooltip formatter → `.tooltip`.
- [ ] **Step 2:** `graph_line_config.inc` — label → `.label`, tooltip → `.tooltip`.
- [ ] **Step 3:** `graph_bar_config.inc` — label → `.label`, tooltip → `.tooltip`.
- [ ] **Step 4:** `graph_area_archive_config.inc` — archive label → `.label`, tooltip → `.tooltip`.
- [ ] **Step 5:** `graph_line_archive_config.inc` — archive label → `.label`, tooltip → `.tooltip`.
- [ ] **Step 6:** `graph_bar_archive_config.inc` — archive label → `.label`, tooltip → `.tooltip`.
- [ ] **Step 7:** `graph_radar_config.inc` — tooltip → `.tooltip` (no label formatter in this file).

- [ ] **Step 8: Verify all replaced**

```bash
cd skins/neowx-material
grep -c "neowxDateFormatter(NEOWX_CHART_FORMAT" graph_area_config.inc graph_line_config.inc graph_bar_config.inc graph_area_archive_config.inc graph_line_archive_config.inc graph_bar_archive_config.inc graph_radar_config.inc
grep -rn "datetime_graph_" graph_area_config.inc graph_line_config.inc graph_bar_config.inc graph_area_archive_config.inc graph_line_archive_config.inc graph_bar_archive_config.inc graph_radar_config.inc
grep -rn "replace(/:MM" graph_area_config.inc graph_line_config.inc graph_bar_config.inc graph_area_archive_config.inc graph_line_archive_config.inc graph_bar_archive_config.inc graph_radar_config.inc
```
Expected: counts → area 2, line 2, bar 2, area_archive 2, line_archive 2, bar_archive 2, radar 1.
The 2nd and 3rd greps return **nothing** (all `datetime_graph_` literals and `:MM` fixes are gone —
the fix now lives only in the helper).

- [ ] **Step 9: Commit**

```bash
git add skins/neowx-material/graph_*config*.inc
git commit -m "Base chart configs read per-page NEOWX_CHART_FORMAT via helper"
```
End the body with the `Co-Authored-By` trailer.

## Task 10: Declare `$neowx_page_scope` in the 7 chart templates

**Files (set the scope on the line immediately before each `#include "js.inc"`):**

| Template                  | scope            | js.inc include ~line |
|---------------------------|------------------|----------------------|
| `index.html.tmpl`         | `current`        | 430                  |
| `yesterday.html.tmpl`     | `yesterday`      | 240                  |
| `week.html.tmpl`          | `week`           | 238                  |
| `month.html.tmpl`         | `month`          | 263                  |
| `month-%Y-%m.html.tmpl`   | `month-archive`  | 266                  |
| `year.html.tmpl`          | `year`           | 241                  |
| `year-%Y.html.tmpl`       | `year-archive`   | 294                  |

For each file, find its `#include "js.inc"` line and insert a scope declaration immediately above it,
matching that file's indentation. Example for `index.html.tmpl`:

Find:
```
        #include "js.inc"
```
Replace with:
```
        #set $neowx_page_scope = 'current'
        #include "js.inc"
```

Use the scope from the table for each file. Do NOT add a scope to other templates (telemetry, almanac,
archive, history) — they intentionally fall back to the global defaults.

- [ ] **Step 1:** `index.html.tmpl` → `current`
- [ ] **Step 2:** `yesterday.html.tmpl` → `yesterday`
- [ ] **Step 3:** `week.html.tmpl` → `week`
- [ ] **Step 4:** `month.html.tmpl` → `month`
- [ ] **Step 5:** `month-%Y-%m.html.tmpl` → `month-archive`
- [ ] **Step 6:** `year.html.tmpl` → `year`
- [ ] **Step 7:** `year-%Y.html.tmpl` → `year-archive`

- [ ] **Step 8: Verify**

```bash
grep -rn "neowx_page_scope" skins/neowx-material/index.html.tmpl skins/neowx-material/yesterday.html.tmpl skins/neowx-material/week.html.tmpl skins/neowx-material/month.html.tmpl "skins/neowx-material/month-%Y-%m.html.tmpl" skins/neowx-material/year.html.tmpl "skins/neowx-material/year-%Y.html.tmpl"
```
Expected: exactly one `#set $neowx_page_scope = '<scope>'` per file with the scope from the table.

- [ ] **Step 9: Commit**

```bash
git add skins/neowx-material/index.html.tmpl skins/neowx-material/yesterday.html.tmpl skins/neowx-material/week.html.tmpl skins/neowx-material/month.html.tmpl "skins/neowx-material/month-%Y-%m.html.tmpl" skins/neowx-material/year.html.tmpl "skins/neowx-material/year-%Y.html.tmpl"
git commit -m "Declare per-page chart-format scope in the 7 chart templates"
```
End the body with the `Co-Authored-By` trailer.

## Task 11: Document `GraphPageFormats` (and archive scopes) in `skin.conf`

**Files:**
- Modify: `skins/neowx-material/skin.conf` (the `[[Formatting]]` documentation block added in Phase 1
  Task 4).

- [ ] **Step 1: Add the `GraphPageFormats` doc + example.** Find:

```
        # datetime_custom_dayonly = ddd DD
        # datetime_custom_full    = ddd DD.MM.YYYY HH:mm
```
Replace with:
```
        # datetime_custom_dayonly = ddd DD
        # datetime_custom_full    = ddd DD.MM.YYYY HH:mm

        # Per-page chart date formats (apply to ALL charts on a page)
        # ---------------------------------------------------------------------
        # Set the x-axis label and/or tooltip date format for EVERY chart
        # (built-in and custom) on a page. Values are named formats from above.
        # A per-chart datetime_label_format / datetime_tooltip_format still
        # overrides these for that one chart.
        #
        # Page scopes: current, yesterday, week, month, month-archive, year,
        # year-archive. The two *-archive scopes inherit from month / year when
        # a key is omitted. With no GraphPageFormats block, charts use the
        # default datetime_graph_label / datetime_graph_archive /
        # datetime_graph_tooltip.
        #
        # [[[GraphPageFormats]]]
        #     [[[[month]]]]
        #         label   = datetime_custom_dayonly
        #         tooltip = datetime_custom_full
        #     [[[[year]]]]
        #         label   = datetime_custom_dayonly
```

- [ ] **Step 2: Update the per-chart scope list comment** to include the archive scopes. Find:

```
        # These keys work at chart level and inside a per-page override
        # subsection ([[[[current]]]], [[[[yesterday]]]], [[[[week]]]],
        # [[[[month]]]], [[[[year]]]]). An unset or unknown name falls back to
        # the chart's normal format. Avoid commas inside a format value.
```
Replace with:
```
        # These keys work at chart level and inside a per-page override
        # subsection ([[[[current]]]], [[[[yesterday]]]], [[[[week]]]],
        # [[[[month]]]], [[[[month-archive]]]], [[[[year]]]], [[[[year-archive]]]]).
        # An unset or unknown name falls back to the chart's normal format.
        # Avoid commas inside a format value.
```

- [ ] **Step 3: Verify parse** (skip if `configobj` unavailable)

```bash
python -c "import configobj; configobj.ConfigObj('skins/neowx-material/skin.conf', file_error=True); print('skin.conf parses OK')"
```
Expected: `skin.conf parses OK`.

- [ ] **Step 4: Commit**

```bash
git add skins/neowx-material/skin.conf
git commit -m "Document GraphPageFormats per-page chart date formats in skin.conf"
```
End the body with the `Co-Authored-By` trailer.

## Task 12: Integrated verification (Phases 2 & 3, manual render)

Behavioral acceptance for the archive scopes and per-page tier. Requires a live WeeWX install.

- [ ] **Step 1: Global no-op.** With **no** `GraphPageFormats` and no per-chart keys, regenerate. On
  any page, confirm the emitted `window.NEOWX_CHART_FORMAT` equals the existing defaults — e.g. on
  `index.html`: `{ label: "dd DD HH:mm", tooltip: "dd DD.MM. HH:mm" }`; on `month-YYYY-MM.html`:
  `{ label: "DD.MM.YY", tooltip: "dd DD.MM. HH:mm" }` (archive label default). All charts render as
  before.
  ```bash
  grep -o 'window.NEOWX_CHART_FORMAT = {[^}]*}' HTML_ROOT/index.html HTML_ROOT/month-*-*.html
  ```

- [ ] **Step 2: Per-page tier.** Add to `[[Formatting]]`:
  ```
  datetime_custom_md = DD.MM
  [[[GraphPageFormats]]]
      [[[[month]]]]
          label   = datetime_custom_md
          tooltip = datetime_custom_md
  ```
  Regenerate. Confirm `month.html` emits `NEOWX_CHART_FORMAT = { label: "DD.MM", tooltip: "DD.MM" }`,
  and in a browser **every** chart on `month.html` — a built-in one (e.g. outTemp), the wind chart,
  and a custom chart with no per-chart override — uses `DD.MM` on the x-axis and tooltip. Other pages
  unchanged.

- [ ] **Step 3: Cascade precedence.** Keep Step 2's page default and add a per-chart override to one
  custom chart on the month page (`[[[customChartOutTemp]]] [[[[month]]]] datetime_label_format =
  datetime_custom_full`, with `datetime_custom_full` defined). Regenerate. Confirm that chart uses the
  per-chart format while its neighbours still use the page default `DD.MM`.

- [ ] **Step 4: Per-page archive inheritance.** With only `[[[[month]]]] label` set under
  `GraphPageFormats` (no `month-archive`), confirm `month-YYYY-MM.html` inherits `DD.MM`. Then add
  `[[[GraphPageFormats]]][[[[month-archive]]]] label = datetime_custom_full`; confirm the archive page
  switches to it while `month.html` keeps `DD.MM`.

- [ ] **Step 5: Per-chart archive merge (Phase 2).** On a custom chart, set `[[[[month]]]] outTemp =
  min, max` and `[[[[month-archive]]]] outTemp = avg`. Regenerate. Confirm `month.html` shows min+max
  series and `month-YYYY-MM.html` shows a single avg series, and that a chart with `[[[[month]]]]` but
  no `[[[[month-archive]]]]` renders identically on both pages (inheritance / backward-compat).

- [ ] **Step 6: No console errors** on every regenerated page; tooltips show real dates (not "Invalid
  date") — this exercises the Task 7 fix in the real ApexCharts tooltip path.

---

## Self-Review (Phases 2 & 3)

**Spec coverage:**
- `month-archive` / `year-archive` whole-block override with base inheritance → Task 6 (`dict()` +
  `.update()`); reserved keys already added in Phase 1. ✓
- `[[[GraphPageFormats]]]` config, 7 scopes, label/tooltip optional → Tasks 8 + 11. ✓
- Per-page resolution with archive→base inheritance and built-in defaults → Task 8 `#set` block. ✓
- `NEOWX_CHART_FORMAT` read by base configs (area/line/bar + archive + radar) → Task 9. ✓
- Per-template scope declaration (7 templates; others default) → Task 10. ✓
- 3-tier cascade, per-chart still wins → Phase 1 Task 3 injection sits on top of the spread whose
  formatter now reads `NEOWX_CHART_FORMAT`; verified in Task 12 Step 3. ✓
- Helper correctness for the real tooltip `(value, optsObject)` signature → Task 7 fix + regression
  test. ✓
- Backward compatible (no config → defaults) → Task 8 default fallbacks; Task 12 Step 1. ✓
- Out of scope (per-chart built-in overrides, value cards, Python strftime) → untouched. ✓

**Placeholder scan:** No TBD/TODO; every code/edit step shows full content and exact commands. ✓

**Type/name consistency:** `NEOWX_CHART_FORMAT.label` / `.tooltip` used identically in Task 8
(emission) and Task 9 (consumption). `$neowx_page_scope` set in Task 10 and read in Task 8. Scope
strings (`current`/`yesterday`/`week`/`month`/`month-archive`/`year`/`year-archive`) match between
Task 8 resolution, Task 10 table, and the spec. `neowxDateFormatter` signature consistent after the
Task 7 fix. ✓
