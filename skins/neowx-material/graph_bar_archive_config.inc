#encoding UTF-8
## +-------------------------------------------------------------------------+
## |    graph_bar_archive_config.inc    JS config for bar charts in archive  |
## +-------------------------------------------------------------------------+

chart: {
    type: 'bar'
},
fill: {
    opacity: 0.1
},
xaxis: {
    type: 'datetime',
    hideOverlappingLabels: true,
    tickAmount: 8,
    labels: {
        formatter: neowxDateFormatter(NEOWX_CHART_FORMAT.label)
    }
},
tooltip: {
    x: {
        formatter: neowxDateFormatter(NEOWX_CHART_FORMAT.tooltip)
    }
},
