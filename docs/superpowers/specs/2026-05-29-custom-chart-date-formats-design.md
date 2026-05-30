# Custom chart date formats — design

**Date:** 2026-05-29
**Status:** Approved for planning
**Scope:** Per-chart date-format overrides for custom charts (`customChart*`), built on a shared
JS helper so a future per-page "all charts" tier is a clean follow-on.

## Problem

Charts render dates using three global moment.js formats defined in `[[Formatting]]`:
`datetime_graph_label` (x-axis), `datetime_graph_tooltip` (hover), and `datetime_graph_archive`
(archive pages). These are baked into the base chart configs (`graph_area_config.inc`,
`graph_line_config.inc`, `graph_bar_config.inc`, and their `_archive_` variants) and shared by
every chart. There is no way to give an individual chart a different date format, nor to vary the
format per page.

## Goal

Let a custom chart declare a named date format — defined in `[[Formatting]]` with the
`datetime_custom_` convention prefix — and override the chart's x-axis labels and/or tooltip date,
at chart level and at the per-page override level (`[[[[current]]]]`, `[[[[yesterday]]]]`,
`[[[[week]]]]`, `[[[[month]]]]`, `[[[[month-archive]]]]`, `[[[[year]]]]`, `[[[[year-archive]]]]`).
The two per-period archive pages additionally support a **whole-block** per-page override
(`column` / `values` / per-field maps / date formats) via the `*-archive` subsections, which inherit
from their base `month` / `year` subsection.

Out of scope for this spec (see Future work): per-page "all charts" overrides, per-chart overrides
for built-in charts, and value-card / Python-strftime date formatting.

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

Evaluated independently for label and tooltip, per page render:

1. Per-page override subsection key. On the two **archive** pages this is the merged subsection, so
   the order within this tier is: `[[[[month-archive]]]]`/`[[[[year-archive]]]]` key →
   `[[[[month]]]]`/`[[[[year]]]]` key. On all other pages it is simply that page's subsection key.
2. Chart top-level key
3. **Unset → inherit the page's existing default** (`datetime_graph_label` / `datetime_graph_tooltip`,
   or `datetime_graph_archive` on archive pages) — i.e. no override is emitted.

A blank value, or a name not found in `[[Formatting]]`, falls back to tier 3 silently (no crash,
no JS error).

The same merge governs the non-date keys (`column`, `values`, per-field maps) on archive pages, since
they all read from the same resolved per-page config.

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

This helper is the foundation for all three tiers of the eventual cascade (see Future work); the
current feature uses it only for per-chart overrides.

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

No changes to value cards, built-in charts, the wind/windvec special charts, or the Python strftime
path.

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
6. Archive merge: add a `[[[[month-archive]]]]` (and `[[[[year-archive]]]]`) subsection that sets a
   *different* `datetime_label_format` and one differing non-date key (e.g. a per-field `outTemp =
   avg`). Regenerate and confirm: (a) the archive page (`month-YYYY-MM.html`, `year-YYYY.html`) uses
   the archive values; (b) the to-date page (`month.html`, `year.html`) still uses the base
   `[[[[month]]]]` / `[[[[year]]]]` values; (c) a chart with `[[[[month]]]]` but **no**
   `[[[[month-archive]]]]` renders identically on both, proving inheritance/backward-compat.

## Future work (informs, not built here)

The 3-tier cascade this enables: **global default → per-page default → per-chart override**, resolved
independently for label and tooltip.

- **Per-page "all charts" tier.** Resolve a per-page label/tooltip format once in each template
  (global → per-page key) and emit it as `window.NEOWX_CHART_FORMAT = { label, tooltip }`. Change the
  base `graph_*_config.inc` formatters from the baked `$Extras.Formatting.datetime_graph_label`
  literal to `neowxDateFormatter(NEOWX_CHART_FORMAT.label)`. Every chart (built-in and custom) then
  picks up the per-page format for free. Config under e.g. `[[Formatting]] [[[GraphPageFormats]]]`
  with `[[[[week]]]] label = …, tooltip = …` subsections.
- **Per-chart override for built-in charts.** Built-in charts have no config block, so this needs a
  name→format map (e.g. `[[Charts]] [[[DateFormats]]] outTemp = datetime_custom_full`) consulted in
  `getChartJsCode`. Heaviest piece; only if per-chart granularity on built-ins is wanted.

> The previously-listed "separate archive scopes" item is now **in scope** — see
> [Supported per-page override subsections](#supported-per-page-override-subsections). It is
> implemented as `[[[[month-archive]]]]` / `[[[[year-archive]]]]` with inheritance from the base
> subsection.

Both reuse the same `neowxDateFormatter` helper, so no rework of this spec's output is required.
