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
