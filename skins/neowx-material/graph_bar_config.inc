#encoding UTF-8
## +-------------------------------------------------------------------------+
## |    graph_bar_config.inc             JS config for default bar charts    |
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
