# Custom chart date formats — design

**Date:** 2026-05-29
**Status:** Phase 1 (per-custom-chart overrides) implemented in PR #1; Phase 2 (`month-archive` /
`year-archive` scopes) and Phase 3 (per-page `[[[GraphPageFormats]]]` tier) approved, pending
implementation.
**Scope:** Configurable chart date formats on a shared `neowxDateFormatter` helper, as a three-tier
cascade: global default → per-page default (all charts) → per-custom-chart override.

## Problem

Charts render dates using three global moment.js formats defined in `[[Formatting]]`:
`datetime_graph_label` (x-axis), `datetime_graph_tooltip` (hover), and `datetime_graph_archive`
(archive pages). These are baked into the base chart configs (`graph_area_config.inc`,
`graph_line_config.inc`, `graph_bar_config.inc`, and their `_archive_` variants) and shared by
every chart. There is no way to give an individual chart a different date format, nor to vary the
format per page.

## Goal

Give two levels of control over chart date formats, both driven by named formats defined in
`[[Formatting]]` (convention prefix `datetime_custom_`):

1. **Per-page default ("all charts").** A `[[[GraphPageFormats]]]` block sets the x-axis label and/or
   tooltip format for **every** chart — built-in *and* custom — on a given page. Supported for all
   seven page scopes (`current`, `yesterday`, `week`, `month`, `month-archive`, `year`,
   `year-archive`).
2. **Per-custom-chart override.** A `customChart*` declares `datetime_label_format` /
   `datetime_tooltip_format` to override its own x-axis labels and/or tooltip, at chart level or
   inside a per-page override subsection. The two per-period archive pages additionally support a
   **whole-block** per-page override (`column` / `values` / per-field maps / date formats) via the
   `*-archive` subsections, which inherit from their base `month` / `year` subsection.

These compose as a cascade — **global default → per-page default → per-chart override** — resolved
independently for label and tooltip (see [Resolution precedence](#resolution-precedence)).

Out of scope for this spec (see Future work): per-chart overrides for **built-in** charts (the
per-page tier covers them collectively, but not individually), and value-card / Python-strftime
date formatting.

## Config schema

### Named formats (`[[Formatting]]`)

Named formats are ordinary keys in the existing `[[Formatting]]` section. Convention: prefix with
`datetime_custom_`. Values are moment.js token strings, same dialect as `datetime_graph_label`.

```ini
[[Formatting]]
    ...
    # Custom named chart date formats (moment.js tokens)
    datetime_custom_dayonly = ddd DD
    datetime_custom_full    = ddd DD.MM.YYYY HH:mm
```

The prefix is a documented convention, **not enforced** — the renderer resolves any key present in
`[[Formatting]]`.

### Per-page default (`[[Formatting]]` → `[[[GraphPageFormats]]]`)

A new optional sub-section sets the date format for **all charts on a page** — built-in and custom
alike. It contains one subsection per page scope, each with optional `label` and `tooltip` keys whose
values are named formats (same as the per-chart keys).

```ini
[Extras]
  [[Formatting]]
    datetime_custom_md  = DD.MM
    datetime_custom_mon = MMM

    [[[GraphPageFormats]]]
        [[[[month]]]]
            label   = datetime_custom_md     # x-axis on every chart on month.html
            tooltip = datetime_custom_md      # tooltip on every chart on month.html
        [[[[year]]]]
            label   = datetime_custom_mon     # tooltip omitted → page default tooltip
```

Recognized page scopes (all seven): `current`, `yesterday`, `week`, `month`, `month-archive`,
`year`, `year-archive`. Each `label` / `tooltip` is independent and optional.

**Defaults and inheritance.** For a given page and slot (label or tooltip):

1. Use `[[[GraphPageFormats]]][[[[<scope>]]]]`'s `label`/`tooltip` if set.
2. For the two archive scopes, otherwise fall back to the base scope's value
   (`month-archive` → `month`, `year-archive` → `year`).
3. Otherwise use the page's existing built-in default: label = `datetime_graph_archive` on the two
   archive pages, `datetime_graph_label` elsewhere; tooltip = `datetime_graph_tooltip` everywhere.

So with no `[[[GraphPageFormats]]]` at all, every page renders exactly as today. (Backward compatible.)

### Per-chart usage (`[[Appearance]]` → `[[[customChart*]]]`)

Two new keys, evaluated independently:

- `datetime_label_format` — overrides the x-axis labels formatter.
- `datetime_tooltip_format` — overrides the hover tooltip formatter.

Both accept a **named key** (a name defined in `[[Formatting]]`). Both are valid at chart top level
and inside a per-page override subsection.

#### Supported per-page override subsections

Seven override scopes are recognized. The to-date templates each read one subsection; the two
**archive** templates read their archive subsection **merged over** the matching to-date subsection
(see "Archive inheritance" below). Any other subsection name is ignored as an override.

| Subsection             | Page                        | Template(s)                | Resolves from                                  |
|------------------------|-----------------------------|----------------------------|------------------------------------------------|
| `[[[[current]]]]`      | Current / "today"           | `index.html.tmpl`          | `current`                                      |
| `[[[[yesterday]]]]`    | Yesterday                   | `yesterday.html.tmpl`      | `yesterday`                                    |
| `[[[[week]]]]`         | Week                        | `week.html.tmpl`           | `week`                                         |
| `[[[[month]]]]`        | Month to-date               | `month.html.tmpl`          | `month`                                        |
| `[[[[month-archive]]]]`| Per-period archive month    | `month-%Y-%m.html.tmpl`    | `month` overlaid by `month-archive`            |
| `[[[[year]]]]`         | Year to-date                | `year.html.tmpl`           | `year`                                         |
| `[[[[year-archive]]]]` | Per-period archive year     | `year-%Y.html.tmpl`        | `year` overlaid by `year-archive`              |

##### Archive inheritance

`[[[[month-archive]]]]` and `[[[[year-archive]]]]` let the per-period archive pages
(`month-%Y-%m.html.tmpl`, `year-%Y.html.tmpl`) carry a **different configuration** from the rolling
to-date pages — and this applies to the **whole per-page block**, not just date formats: `column`,
`values`, per-field column maps, and the `datetime_*_format` keys.

The archive subsection **inherits** from its base subsection: the archive template's effective
per-page config is `[[[[month]]]]` (or `[[[[year]]]]`) **overlaid by** `[[[[month-archive]]]]` (or
`[[[[year-archive]]]]`). Any key absent from the archive subsection falls through to the base
subsection, then to the chart top-level, then to the default. Consequently:

- A config with **no** archive subsection behaves exactly as before — archive pages use the `month` /
  `year` subsection. (Backward compatible.)
- You only specify in the archive subsection what should **differ** on the archive page.

```ini
[[[customChartOutTemp]]]
    charttype = area
    values    = outTemp, dewpoint
    column    = avg
    [[[[month]]]]
        outTemp = min, max
        datetime_label_format = datetime_custom_dayonly
    [[[[month-archive]]]]
        # inherits values/outTemp=min,max from [[[[month]]]]; overrides only the label format
        datetime_label_format = datetime_custom_monthonly
```

##### Worked example: template-driven month / year config

A full, self-contained example showing how the to-date subsections (`month`, `year`) and their
archive counterparts produce different output on the four month/year templates. The motivating idea:
rolling "to-date" pages omit the year (the period is implied), while the fixed historical archive
pages spell the year out.

```ini
[Extras]
  [[Formatting]]
    # named moment.js formats used below
    datetime_custom_md  = DD.MM         # day + month, no year   (rolling pages)
    datetime_custom_mdy = DD.MM.YYYY    # day + month + year      (fixed archive pages)
    datetime_custom_mon = MMM           # month abbreviation only

  [[Appearance]]
    [[[customChartOutTemp]]]
        title     = Outdoor Temperature
        charttype = area
        values    = outTemp, dewpoint
        column    = avg

        # --- Month ---
        [[[[month]]]]                    # drives month.html (rolling, current month)
            outTemp = min, max
            datetime_label_format   = datetime_custom_md
            datetime_tooltip_format = datetime_custom_md
        [[[[month-archive]]]]            # drives month-%Y-%m.html (a specific past month)
            # inherits values + outTemp=min,max from [[[[month]]]];
            # only the formats differ — show the year because the period is historical
            datetime_label_format   = datetime_custom_mdy
            datetime_tooltip_format = datetime_custom_mdy

        # --- Year ---
        [[[[year]]]]                     # drives year.html (rolling, current year)
            outTemp = min, max
            datetime_label_format = datetime_custom_mon
        [[[[year-archive]]]]             # drives year-%Y.html (a specific past year)
            # inherits the MMM label from [[[[year]]]];
            # overrides only the series: a single avg line instead of min/max
            outTemp = avg
```

Resulting render per template:

| Template            | Series for `outTemp`     | x-axis label / tooltip      | Source                                              |
|---------------------|--------------------------|-----------------------------|-----------------------------------------------------|
| `month.html`        | min + max                | `DD.MM`                     | `[[[[month]]]]`                                     |
| `month-%Y-%m.html`  | min + max *(inherited)*  | `DD.MM.YYYY` *(overridden)* | `[[[[month]]]]` ⊕ `[[[[month-archive]]]]`           |
| `year.html`         | min + max                | `MMM` (label), default tooltip | `[[[[year]]]]`                                  |
| `year-%Y.html`      | avg *(overridden)*       | `MMM` *(inherited)*, default tooltip | `[[[[year]]]]` ⊕ `[[[[year-archive]]]]`     |

This shows both axes of the merge: the archive month page inherits the **series** and overrides the
**date format**, while the archive year page inherits the **date format** and overrides the
**series** — each archive page changing only what it declares and inheriting the rest.

```ini
[[[customChartOutTemp]]]
    title     = Outdoor Temperature
    charttype = area
    values    = outTemp, dewpoint
    column    = avg
    datetime_label_format   = datetime_custom_dayonly   # x-axis labels (all pages)
    datetime_tooltip_format = datetime_custom_full       # hover tooltip (all pages)
    [[[[week]]]]
        outTemp = min, max
        datetime_label_format = datetime_custom_full     # per-page override of the label
```

### Resolution precedence

The effective label/tooltip format for a chart is a three-tier cascade, evaluated independently for
label and tooltip, per page render. Highest priority first:

1. **Per-chart override** (custom charts only). The `datetime_label_format` / `datetime_tooltip_format`
   resolved from the chart's per-page subsection then chart top-level. On the two **archive** pages the
   subsection is the merged one, so within this tier the order is
   `[[[[month-archive]]]]`/`[[[[year-archive]]]]` key → `[[[[month]]]]`/`[[[[year]]]]` key → chart
   top-level key. Emitted as an explicit per-chart `xaxis`/`tooltip` override.
2. **Per-page default** — `[[[GraphPageFormats]]][[[[<scope>]]]]`'s `label`/`tooltip`, with
   archive→base inheritance (`month-archive` → `month`, `year-archive` → `year`). Applies to every
   chart on the page (built-in and custom) via the shared base config.
3. **Page built-in default** — `datetime_graph_label` (or `datetime_graph_archive` on archive pages)
   for the axis, `datetime_graph_tooltip` for the tooltip.

A blank value, or a name not found in `[[Formatting]]`, is treated as unset and falls through to the
next tier silently (no crash, no JS error). With no new config at all, every chart resolves to tier 3
— identical to today.

Tiers 2 and 3 are realized as the page's `NEOWX_CHART_FORMAT` (which the base chart config reads);
tier 1 is the per-chart `xaxis`/`tooltip` block emitted on top of the spread (see Renderer design).
For custom charts, the per-chart whole-block merge also governs the non-date keys (`column`,
`values`, per-field maps) on archive pages, since they read from the same resolved per-page config.

## Renderer design

### Shared helper (`js.inc`)

Add one helper, included on every chart page via the existing `#include "js.inc"`:

```js
// Returns a moment formatter for the given format string.
// Centralizes the ":MM" -> ":mm" minutes-token fix currently duplicated across the
// graph_*_config.inc files.
function neowxDateFormatter(fmt) {
    fmt = fmt.replace(/:MM/g, ':mm');
    return function (val, timestamp) {
        var ts = (typeof timestamp !== 'undefined') ? timestamp : val;
        return moment.unix(ts).format(fmt);
    };
}
```

This helper is the foundation for **all three tiers** of the cascade — the base config formatters
(tiers 2/3) and the per-chart override (tier 1) both call it.

### Per-page tier — `NEOWX_CHART_FORMAT` + base config rewrite

The per-page default is realized once per page as a small JS object that the base chart configs read.

**1. Each template declares its page scope before including `js.inc`:**

```cheetah
#set $neowx_page_scope = 'month'      ## index→current, yesterday, week, month, year;
#include "js.inc"                      ## month-%Y-%m→month-archive, year-%Y→year-archive
```

**2. `js.inc` resolves the page's effective formats and emits them** (one place; runs for every page
that includes `js.inc`, so pages without charts and pages with no `GraphPageFormats` still get valid
defaults):

```cheetah
## scope, with archive→base fallback for the GraphPageFormats lookup
#set $gpf = $Extras.Formatting.get('GraphPageFormats', {})
#set $scope = $getVar('neowx_page_scope', '')
#set $base_scope = 'month' if $scope == 'month-archive' else ('year' if $scope == 'year-archive' else $scope)
#set $is_archive = $scope in ('month-archive', 'year-archive')
## label/tooltip named-key: scope → base scope (archive only) → ''
#set $lbl_key = $gpf.get($scope, {}).get('label',   $gpf.get($base_scope, {}).get('label',   '')) if $scope else ''
#set $tip_key = $gpf.get($scope, {}).get('tooltip', $gpf.get($base_scope, {}).get('tooltip', '')) if $scope else ''
## resolve to a moment string, else the page built-in default
#set $lbl_default = $Extras.Formatting.datetime_graph_archive if $is_archive else $Extras.Formatting.datetime_graph_label
#set $lbl_fmt = $Extras.Formatting.get($lbl_key, '') if $lbl_key else ''
#set $tip_fmt = $Extras.Formatting.get($tip_key, '') if $tip_key else ''
#if not $lbl_fmt
    #set $lbl_fmt = $lbl_default
#end if
#if not $tip_fmt
    #set $tip_fmt = $Extras.Formatting.datetime_graph_tooltip
#end if
```
```js
window.NEOWX_CHART_FORMAT = { label: "$lbl_fmt", tooltip: "$tip_fmt" };
```

(The `', '.join(...)` list-coercion used for the per-chart keys applies here too if a named format
value contains a comma.)

**3. The base config `.inc` formatters read `NEOWX_CHART_FORMAT`** instead of baking the literal.
Affected files: `graph_area_config.inc`, `graph_line_config.inc`, `graph_bar_config.inc`, their
three `_archive_` variants, and `graph_radar_config.inc` (tooltip only). Each formatter changes from:

```js
formatter: function (val, timestamp) {
    var fmt = "$Extras.Formatting.datetime_graph_label";   // (or _archive / _tooltip)
    fmt = fmt.replace(/:MM/g, ':mm');
    return moment.unix(timestamp).format(fmt);
}
```
to:
```js
formatter: neowxDateFormatter(NEOWX_CHART_FORMAT.label)     // tooltip uses .tooltip
```

Because every chart — built-in, special (wind/windvec), and custom — spreads `...graph_${type}_config`,
they all pick up the page format with no per-chart edits. The archive `.inc` files map their label
default into `NEOWX_CHART_FORMAT.label` (js.inc seeds it with `datetime_graph_archive` for archive
scopes), so existing archive behavior is preserved when no `GraphPageFormats` is set.

### Custom-chart loop changes (7 templates)

Templates with a `customChart*` render loop, with how each builds its `cc_page` (the resolved
per-page config the rest of the loop reads):

| Template               | `cc_page` source                                              |
|------------------------|---------------------------------------------------------------|
| `index.html.tmpl`      | `cc.get('current', {})`                                       |
| `yesterday.html.tmpl`  | `cc.get('yesterday', {})`                                     |
| `week.html.tmpl`       | `cc.get('week', {})`                                          |
| `month.html.tmpl`      | `cc.get('month', {})`                                         |
| `month-%Y-%m.html.tmpl`| `cc.get('month', {})` overlaid by `cc.get('month-archive', {})` |
| `year.html.tmpl`       | `cc.get('year', {})`                                          |
| `year-%Y.html.tmpl`    | `cc.get('year', {})` overlaid by `cc.get('year-archive', {})`   |

The five to-date templates are unchanged from the original feature. The two **archive** templates
build `cc_page` as a shallow merge so the archive subsection inherits from the base subsection:

```cheetah
## month-%Y-%m.html.tmpl (year-%Y.html.tmpl uses 'year' / 'year-archive')
#set $cc_page = dict($cc.get('month', {}))
#silent $cc_page.update($cc.get('month-archive', {}))
```

`dict(...)` + `.update(...)` overlays the archive subsection's keys (scalars, lists, and per-field
column keys) onto the base subsection's, so archive-only keys win and unset keys are inherited. The
page subsections contain only scalar/list values (no deeper nesting), so a shallow merge is correct.
Because every downstream read goes through `cc_page.get(...)`, this single change gives the archive
pages independent `column` / `values` / per-field maps / `datetime_*_format` with no other edits.

In each loop:

1. **Reserve the new keys.** Add `datetime_label_format` and `datetime_tooltip_format` to the
   reserved-key list (`CC_RESERVED`) and to the `cc_field_map` exclusion list, so they are not
   mistaken for per-field column assignments.

2. **Resolve formats.** For label and tooltip independently:
   - read the key from `cc_page` (the resolved per-page config — already archive-merged where
     applicable), else from `cc` (chart top level);
   - if non-empty, look it up in `$Extras.Formatting` to get the moment string;
   - empty / missing → leave unset (no override emitted).

3. **Inject overrides.** When a format resolves, emit `xaxis` / `tooltip` keys into the ApexCharts
   options object **after** the `...graph_${cc_type}_config` spread, merging so only the formatter
   is replaced (preserving `type: 'datetime'`, `tickAmount`, etc.):

   ```js
   // emitted only if the label format resolved
   xaxis: {
       ...graph_${cc_type}_config.xaxis,
       labels: {
           ...graph_${cc_type}_config.xaxis.labels,
           formatter: neowxDateFormatter("$cc_label_fmt")
       }
   },
   // emitted only if the tooltip format resolved
   tooltip: {
       ...graph_${cc_type}_config.tooltip,
       x: {
           ...graph_${cc_type}_config.tooltip.x,
           formatter: neowxDateFormatter("$cc_tooltip_fmt")
       }
   },
   ```

   Object-literal key order guarantees these override the spread's defaults. The base configs for
   all custom-chart types (`area`, `bar`, `line`) and their archive variants all expose
   `xaxis.labels.formatter` and `tooltip.x.formatter`, so the merge is uniform across pages and types.
   Because the spread's formatter now reads `NEOWX_CHART_FORMAT` (tier 2/3), this per-chart block
   correctly sits on top as tier 1.

The custom-chart **loop** changes touch only custom charts. Built-in and wind/windvec charts are
affected only indirectly, and only by the **per-page tier**, through the shared base-config rewrite —
their own rendering code is unchanged. No changes to value cards or the Python strftime path.

## Error handling

- Unknown or blank named key → no override emitted; the page default formatter stands. No exception
  in the Cheetah render and no `undefined` reaching `neowxDateFormatter`.
- The `:MM` → `:mm` fix is applied centrally in the helper, matching existing behavior.

## Verification

No automated test harness exists in this skin; verification is manual.

1. Add `datetime_custom_*` formats to `[[Formatting]]` and `datetime_label_format` /
   `datetime_tooltip_format` to a custom chart, including a per-page override.
2. Regenerate the report (weewx report generation).
3. Grep generated page JS to confirm `neowxDateFormatter("<expected moment string>")` is emitted for
   the right chart on the right page (current/week/month/year + the `-%Y` archive pages).
4. Open the pages in a browser; confirm x-axis labels and tooltip render with the chosen format, and
   that charts without the keys are unchanged.
5. Negative case: reference an undefined key → chart falls back to the default format, no console error.
6. Archive merge (per-chart): add a `[[[[month-archive]]]]` (and `[[[[year-archive]]]]`) subsection
   that sets a *different* `datetime_label_format` and one differing non-date key (e.g. a per-field
   `outTemp = avg`). Regenerate and confirm: (a) the archive page (`month-YYYY-MM.html`,
   `year-YYYY.html`) uses the archive values; (b) the to-date page (`month.html`, `year.html`) still
   uses the base `[[[[month]]]]` / `[[[[year]]]]` values; (c) a chart with `[[[[month]]]]` but **no**
   `[[[[month-archive]]]]` renders identically on both, proving inheritance/backward-compat.
7. Per-page tier: add `[[[GraphPageFormats]]][[[[month]]]] label = datetime_custom_md`. Regenerate and
   confirm `window.NEOWX_CHART_FORMAT = { label: "DD.MM", … }` is emitted on `month.html`, and that
   **every** chart there — a built-in one (e.g. outTemp), the wind chart, and a custom chart with no
   per-chart override — uses `DD.MM` on the x-axis. Confirm other pages are unchanged.
8. Cascade precedence: with the tier-7 page default in place, add a per-chart
   `datetime_label_format` to one custom chart on the month page and confirm that chart uses the
   per-chart format while its neighbours still use the page default.
9. Per-page archive inheritance: set only `[[[GraphPageFormats]]][[[[month]]]] label` (no
   `month-archive`) and confirm `month-YYYY-MM.html` inherits it; then add
   `[[[[month-archive]]]] label` and confirm the archive page switches to it while `month.html` keeps
   the base.
10. Global no-op: with **no** `GraphPageFormats` and no per-chart keys, confirm every page's emitted
    `NEOWX_CHART_FORMAT` equals the existing defaults (`datetime_graph_label`/`_archive` +
    `datetime_graph_tooltip`) and all charts render exactly as before.

## Future work (informs, not built here)

- **Per-chart override for built-in charts.** Built-in charts have no config block, so giving an
  *individual* built-in chart its own format needs a name→format map (e.g.
  `[[Charts]] [[[DateFormats]]] outTemp = datetime_custom_full`) consulted in `getChartJsCode`. The
  per-page tier already covers built-in charts *collectively*; this is only for per-chart granularity
  on built-ins. Would reuse the same `neowxDateFormatter` helper, so no rework of this spec's output.
