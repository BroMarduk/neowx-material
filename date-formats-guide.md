# Date & Time Formats Guide

A practical guide to controlling how dates and times appear on your NeoWX Material charts and cards.

Everything here lives in **`skin.conf`** under `[Extras] → [[Formatting]]` and (for charts)
`[[Appearance]]`. No template editing required. After any change, regenerate your report
(`weectl report run` or wait for the next archive cycle) and hard-refresh the page.

---

## Overview

- **Charts** (the x-axis labels and the hover tooltip) use **moment.js** format tokens - uppercase
  `DD`, `MM`, `YYYY`, `HH`, lowercase `mm` for minutes.
- **Cards** (the little "min/max at 14:32" time under a value) use **Python strftime** - `%d`, `%m`,
  `%Y`, `%H`, `%M`. Different language entirely.
- You can change formats at three levels for charts and one level for cards:

| What you want to change | Where |
|---|---|
| One custom chart's dates | `datetime_label_format` / `datetime_tooltip_format` on that chart |
| **Every** chart on a page (e.g. the whole Month page) | `[[[GraphPageFormats]]]` |
| Every **card** on a page | `[[[CardPageFormats]]]` |
| The global default for all charts | `datetime_graph_label` / `_tooltip` / `_archive` (already in your `skin.conf`) |

You define **named formats** once and refer to them by name. By convention:

- `datetime_custom_graph_*` → a **moment** string (for charts)
- `datetime_custom_card_*` → a **strftime** string (for cards)

Keeping the prefixes straight matters: feed a moment string to a card (or vice-versa) and you'll get
literal gibberish on the page.

---

## Token Cheat-Sheet

**Charts (moment.js)** - examples: `dd DD HH:mm`, `DD.MM.YYYY`, `MMM`

| Token | Meaning | Example |
|---|---|---|
| `DD` | day of month, 2-digit | `07` |
| `ddd` / `dddd` | weekday short / long | `Mon` / `Monday` |
| `MM` | month number | `03` |
| `MMM` / `MMMM` | month name short / long | `Mar` / `March` |
| `YY` / `YYYY` | year 2- / 4-digit | `26` / `2026` |
| `HH` | hour, 24h | `14` |
| `hh` | hour, 12h | `02` |
| `mm` | **minutes** (lowercase!) | `05` |
| `A` / `a` | AM/PM / am-pm marker | `PM` / `pm` |

> **Drop a leading zero** by using the single-letter token instead of the double: `D` `M` `H` `h` `m`
> (instead of `DD` `MM` `HH` `hh` `mm`). Example: `D.M.YYYY H:mm` → `7.3.2026 9:05`.

> ⚠️ In moment, **`MM` is the month** and **`mm` is minutes**. Writing `HH:MM` by mistake is so
> common that the skin auto-corrects `:MM` → `:mm` for you - but it doing it right is easy too.

**Cards (Python strftime)** - examples: `%H:%M`, `%a %d %H:%M`, `%d.%m.%Y %H:%M`

| Token | Meaning | Example |
|---|---|---|
| `%d` | day of month | `07` |
| `%a` / `%A` | weekday short / long | `Mon` / `Monday` |
| `%m` | month number | `03` |
| `%b` / `%B` | month name short / long | `Mar` / `March` |
| `%y` / `%Y` | year 2- / 4-digit | `26` / `2026` |
| `%H` | hour, 24h | `14` |
| `%I` | hour, 12h | `02` |
| `%M` | **minutes** | `05` |
| `%p` | AM/PM marker | `PM` |

> **Drop a leading zero** on Linux/glibc (the usual WeeWX host) by prefixing the token with `-`:
> `%-d` `%-m` `%-H` `%-I` (instead of `%d` `%m` `%H` `%I`). Example: `%-d.%-m.%Y %-H:%M` → `7.3.2026 9:05`.
> This is a GNU `strftime` extension and may not work on non-Linux hosts.

> **A note on regional ordering.** The examples in this guide use the European convention - day-first
> and a 24-hour clock (`DD.MM.YYYY HH:mm` for charts, `%d.%m.%Y %H:%M` for cards). US-style is equally
> supported - just swap the tokens to month-first with a 12-hour clock and an AM/PM marker, e.g.
> `ddd MM/DD/YYYY hh:mm A` (chart) or `%a %m/%d/%Y %I:%M %p` (card). Pick whichever you prefer; the
> skin doesn't care which ordering you use.

---

## Step 1 - Define your Named Formats

Add these under `[Extras] → [[Formatting]]` (next to the existing `datetime_graph_*` keys):

```ini
[Extras]
    [[Formatting]]
        # ... existing datetime_graph_label / _tooltip / _archive stay as-is ...

        # Chart (moment.js) named formats
        datetime_custom_graph_dayonly = ddd DD
        datetime_custom_graph_full    = ddd DD.MM.YYYY HH:mm
        datetime_custom_graph_month   = MMM

        # Card (strftime) named formats
        datetime_custom_card_short = %H:%M
        datetime_custom_card_full  = %a %d.%m.%Y %H:%M
```

> Avoid commas inside a format value - the config parser treats a comma as a list separator.
> `%a, %d %b` will misbehave; use `%a %d %b` instead.

Now you can reference these by name in the examples below.

---

## Example 1 - Change One Custom Chart's Dates

On any `customChart*` in `[[Appearance]]`, add either or both keys:

```ini
[Extras]
    [[Appearance]]
        [[[customChartOutTemp]]]
            title     = Outdoor Temperature
            charttype = area
            values    = outTemp, dewpoint
            column    = avg
            datetime_label_format   = datetime_custom_graph_dayonly   # x-axis labels
            datetime_tooltip_format = datetime_custom_graph_full        # hover tooltip
```

Set just one if you only want to change the axis *or* the tooltip. Anything you leave out keeps the
normal format for that page.

### Vary it per page

Inside a custom chart you can override per page using the page sub-sections you already use for
`column` / `values`:

```ini
        [[[customChartOutTemp]]]
            charttype = area
            values    = outTemp, dewpoint
            column    = avg
            datetime_label_format = datetime_custom_graph_dayonly      # default for all pages
            [[[[week]]]]
                outTemp  = min, max
                datetime_label_format = datetime_custom_graph_full     # …but full detail on the Week page
```

Page sub-sections: `[[[[current]]]]`, `[[[[yesterday]]]]`, `[[[[week]]]]`, `[[[[month]]]]`,
`[[[[month-archive]]]]`, `[[[[year]]]]`, `[[[[year-archive]]]]`.

---

## Example 2 - Change **Every** Chart on a Page

This is the one most people actually want: "make the whole Month page use a shorter date." Use
`[[[GraphPageFormats]]]` under `[[Formatting]]`. It hits **all** charts on the page - the built-in
ones (outTemp, rain, wind…) *and* your custom charts:

```ini
[Extras]
    [[Formatting]]
        [[[GraphPageFormats]]]
            [[[[month]]]]
                label   = datetime_custom_graph_dayonly    # x-axis on every chart on the Month page
                tooltip = datetime_custom_graph_full
            [[[[year]]]]
                label   = datetime_custom_graph_month       # just "Mar", "Apr", … on the Year page
```

- Scopes: the same seven page names as above.
- Set `label`, `tooltip`, or both. Omit one and that slot keeps the page's normal format.
- A per-chart format (Example 1) still wins over this for that one chart.

---

## Example 3 - Make the Archive Pages Different

The **to-date** pages (`month.html`, `year.html` - the current month/year, always updating) and the
**archive** pages (`month-YYYY-MM.html`, `year-YYYY.html` - a specific finished period) are separate.
("To-date" is WeeWX's own term - these come from the `[[ToDate]]` report group.)

A common wish: the to-date pages omit the year (it's obvious), but the fixed archive pages spell it
out. The `*-archive` scopes inherit from their base scope, so you only specify what differs:

```ini
        [[[GraphPageFormats]]]
            [[[[month]]]]
                label = datetime_custom_graph_dayonly       # "Mon 07"  - month to date
            [[[[month-archive]]]]
                label = datetime_custom_graph_full          # "Mon 07.03.2026 14:05" - archived month
```

If you set only `[[[[month]]]]`, the archive month page inherits it. The same pattern works for
custom charts via the `[[[[month-archive]]]]` / `[[[[year-archive]]]]` sub-sections (Example 1), where
it also covers `column` / `values` - e.g. show min/max on the to-date page but a single average on the
archived page.

---

## Example 4 - Change the Card Times

The small "high 24.3° at **14:32**" times under each value card are controlled by
`[[[CardPageFormats]]]`. Remember: **strftime**, so `datetime_custom_card_*` names.

```ini
[Extras]
    [[Formatting]]
        [[[CardPageFormats]]]
            month = datetime_custom_card_full      # every card's min/max time on the Month pages
            year  = datetime_custom_card_full
```

- Same seven scopes; archive scopes inherit from `month` / `year`.
- Cards are page-level only - there's no per-individual-card override.
- The Telemetry page's cards follow the `current` scope.

---

## How the Layers Combine (Precedence)

For a chart's label or tooltip, the skin picks the first of these that's set:

1. **Per-chart** key on that custom chart (Example 1) - most specific, wins.
2. **Per-page** `GraphPageFormats` for that page (Example 2).
3. The **global default** (`datetime_graph_label` / `_tooltip`, or `datetime_graph_archive` for the
   axis on archive pages).

Cards are simpler: **per-page `CardPageFormats`** → the page's built-in default.

**Built-in defaults**, for reference (these already ship in your `skin.conf`):

| | Current / yesterday | Week | Month | Year | Archive month/year |
|---|---|---|---|---|---|
| Chart axis | `datetime_graph_label` | ← | ← | ← | `datetime_graph_archive` |
| Chart tooltip | `datetime_graph_tooltip` | ← | ← | ← | ← |
| Card time | `datetime_today` | `datetime` | `datetime` | `datetime_archive` | `datetime` / `datetime_archive` |

**Do nothing and nothing changes.** If you add no `GraphPageFormats`, no `CardPageFormats`, and no
per-chart keys, every chart and card renders exactly as it does today.

---

## Customizing the Built-in Defaults

The "defaults" above aren't magic - they're ordinary keys that already live in your `skin.conf` under
`[Extras] → [[Formatting]]`. The `GraphPageFormats` / `CardPageFormats` / per-chart layers sit *on top*
of them; if you'd rather change the **baseline for the whole site** (one house style, no per-page
fiddling), just edit these keys directly.

| Key | Dialect | Ships as | What it affects |
|---|---|---|---|
| `datetime_graph_label` | moment | `dd DD HH:mm` | Chart **x-axis** labels on the non-archive pages (current, yesterday, week, month, year) |
| `datetime_graph_tooltip` | moment | `dd DD.MM. HH:mm` | Chart **hover tooltip** on **every** page (including archive) |
| `datetime_graph_archive` | moment | `DD.MM.YY` | Chart **x-axis** labels on the **archive** pages (`month-YYYY-MM`, `year-YYYY`) |
| `datetime_today` | strftime | `%H:%M` | **Card** min/max times on current, yesterday, telemetry |
| `datetime` | strftime | `%a %d %H:%M` | **Card** min/max times on week, month, month-archive |
| `datetime_archive` | strftime | `%d.%m. %H:%M` | **Card** min/max times on year, year-archive |

How they fit the cascade:

- The three `datetime_graph_*` keys are **tier 3** for charts - the final fallback when no
  `GraphPageFormats` and no per-chart key applies. Editing one shifts the baseline for every chart
  that hasn't been overridden.
- The three card keys (`datetime_today` / `datetime` / `datetime_archive`) are the per-page **card
  default** - the fallback when no `CardPageFormats` entry applies.

> ⚠️ **`datetime` does double duty.** Besides the week/month card times above, `datetime` also formats
> the header's **"last updated" timestamp** (refreshed live when MQTT is enabled). Two ways to keep
> them apart: use `CardPageFormats` (Example 4) to restyle only the cards and leave `datetime` alone,
> or set the optional **`datetime_updated`** key to give the timestamp its own format (it falls back
> to `datetime` when unset, so existing setups are unaffected).

What these keys do **not** control:

- **The header's date and time rows** use their own separate `date` and `time` keys (not in the table
  above). Edit those to restyle the header without touching cards or charts.
- **Sunrise / sunset** (and moon / twilight) times aren't formatted here at all - they come from
  WeeWX's almanac and are controlled in `weewx.conf` under `[Units] [[TimeFormats]]`.

**Rule of thumb:** edit the defaults for a single global look; use the per-page / per-chart layers
(Examples 1-4) for exceptions. The defaults are also your reference for *what a page currently does* -
copy the relevant default into a `datetime_custom_*` name as a starting point and tweak from there.

---

## A Complete Worked Example

Goal: short dates on the Month page's charts, full dates on the archived month, nicer card times
site-wide on Month - and one specific custom chart (Outdoor Temperature) that keeps a bare `HH:mm`
tooltip because you stare at it all day and don't need the date repeated.

```ini
[Extras]
    [[Formatting]]
        datetime_custom_graph_short = ddd DD
        datetime_custom_graph_full  = ddd DD.MM.YYYY HH:mm
        datetime_custom_graph_time  = HH:mm
        datetime_custom_graph_month = MMM
        datetime_custom_card_full   = %a %d.%m.%Y %H:%M

        # Per-page: every chart on the Month pages
        [[[GraphPageFormats]]]
            [[[[month]]]]
                label   = datetime_custom_graph_short
                tooltip = datetime_custom_graph_full
            [[[[month-archive]]]]
                label   = datetime_custom_graph_full     # archived month spells out the date

        # Per-page: every card on the Month pages
        [[[CardPageFormats]]]
            month = datetime_custom_card_full            # inherited by month-archive too

    [[Appearance]]
        # Per-chart: this one chart overrides just its tooltip, on every page
        [[[customChartOutTemp]]]
            title     = Outdoor Temperature
            charttype = area
            column    = avg
            values    = outTemp, dewpoint
            datetime_tooltip_format = datetime_custom_graph_time   # wins over GraphPageFormats
            [[[[year]]]]
                datetime_label_format = datetime_custom_graph_month   # axis = "Mar", "Apr", … on year.html
            [[[[year-archive]]]]
                datetime_label_format = datetime_custom_graph_full    # archived year spells the date out
```

Result, on `month.html`:

- **Every chart** has axes reading `Mon 07` and tooltips reading `Mon 07.03.2026 14:05` (the page tier)…
- **…except the Outdoor Temperature chart**, whose tooltip reads just `14:05` - its per-chart
  `datetime_tooltip_format` wins, while its axis still follows the page (`Mon 07`) because it didn't
  override the label.
- **Card** min/max times read `Mon 07.03.2026 14:05`.

On `month-YYYY-MM.html` the axis spells out the full date (the `month-archive` override), the
Outdoor Temperature tooltip is still `14:05` (per-chart formats apply on every page), and the cards
inherit the `month` card format.

On the Year pages, only the Outdoor Temperature chart changes (no `GraphPageFormats` is set for
`year`), thanks to its per-chart sub-sections:

- `year.html` - its axis reads `Mar`, `Apr`, … (the `[[[[year]]]]` label override), tooltip still `14:05`.
- `year-YYYY.html` - its axis spells out the full date (the `[[[[year-archive]]]]` override), tooltip
  still `14:05`. Every other chart on the Year pages keeps the global defaults.

Pages and charts you didn't touch render exactly as before.

---

## Troubleshooting

- **The whole label is literal text like `datetime_custom_graph_full`** - you referenced a name that
  doesn't exist in `[[Formatting]]`, or there's a typo. Names are case-sensitive.
- **A card shows `dd DD HH:mm` literally** - you used a *graph* (moment) name in `CardPageFormats`.
  Cards need a `datetime_custom_card_*` (strftime) name.
- **Minutes show the month** - you wrote `MM` instead of `mm` in a moment format somewhere other than
  after a colon (the auto-fix only covers `:MM`).
- **Nothing changed** - did you regenerate the report and hard-refresh? Is the chart actually in
  `charts_order`?  Try deleting the html file and letting the report generator recreate it
