#encoding UTF-8
## ---------------------------------------------------------------------------
## Resolve $card_datetime (global) for value-card min/max times on a page.
## Include once per page AFTER setting (as #set global):
##   $card_scope   - page scope: current, yesterday, week, month, month-archive,
##                   year, year-archive, telemetry
##   $card_default - fallback strftime string (e.g. $Extras.Formatting.datetime)
## Looks up CardPageFormats[$card_scope] (a datetime_custom_card_* name); the two
## *-archive scopes fall back to their base (month/year); then $card_default.
## ---------------------------------------------------------------------------
#set $cpf = $Extras.Formatting.get('CardPageFormats', {})
#set $card_base = 'month' if $card_scope == 'month-archive' else ('year' if $card_scope == 'year-archive' else '')
#set $card_key = $cpf.get($card_scope, $cpf.get($card_base, '')) if $card_base else $cpf.get($card_scope, '')
#set global $card_datetime = $Extras.Formatting.get($card_key, $card_default) if $card_key else $card_default
#if isinstance($card_datetime, list)
    #set global $card_datetime = ', '.join($card_datetime)
#end if
