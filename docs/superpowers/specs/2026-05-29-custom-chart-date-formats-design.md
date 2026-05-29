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
at chart level and at the per-page override level (`[[[[week]]]]`, `[[[[month]]]]`, `[[[[year]]]]`,
`[[[[current]]]]`, `[[[[yesterday]]]]`).

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

1. Per-page override subsection key (e.g. `[[[[week]]]] datetime_label_format`)
2. Chart top-level key
3. **Unset → inherit the page's existing default** (`datetime_graph_label` / `datetime_graph_tooltip`,
   or `datetime_graph_archive` on archive pages) — i.e. no override is emitted.

A blank value, or a name not found in `[[Formatting]]`, falls back to tier 3 silently (no crash,
no JS error).

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

Templates with a `customChart*` render loop, with their per-page override key:

| Template               | page key    |
|------------------------|-------------|
| `index.html.tmpl`      | `current`   |
| `yesterday.html.tmpl`  | `yesterday` |
| `week.html.tmpl`       | `week`      |
| `month.html.tmpl`      | `month`     |
| `month-%Y-%m.html.tmpl`| `month`     |
| `year.html.tmpl`       | `year`      |
| `year-%Y.html.tmpl`    | `year`      |

In each loop:

1. **Reserve the new keys.** Add `datetime_label_format` and `datetime_tooltip_format` to the
   reserved-key list (`CC_RESERVED`) and to the `cc_field_map` exclusion list, so they are not
   mistaken for per-field column assignments.

2. **Resolve formats.** For label and tooltip independently:
   - read the key from `cc_page` (page override), else from `cc` (chart top level);
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

Both reuse the same `neowxDateFormatter` helper, so no rework of this spec's output is required.
