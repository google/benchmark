#!/usr/bin/env python

import argparse
from argparse import ArgumentParser
import json
from datetime import datetime
import math
import operator
import os
import sys
import re

def create_parser():
    parser = ArgumentParser(description='Generate an HTML document containing interactive charts composed of data from multiple google benchmark JSON outputs')
    parser.add_argument('-d', '--directory', help="Directory containing benchmark result json files to process")
    parser.add_argument('-m', '--metric', help="The benchmark metric to track", default="real_time")
    parser.add_argument('-t', '--charttype', help="chart.js chart type", default="line")
    parser.add_argument('-o', '--outputfile', help="The html file to output to", default="charts.html")
    parser.add_argument('-n', '--maxchartentries', help="Max entries in a single chart", default=-1)
    parser.add_argument('-f', '--filterregex', help="Benchmark names that match this regex will be included in the html output", default=r'.*')
    parser.add_argument('-c', '--chartmatchregex', help="Regex used to format benchmark name into chart title, will merge charts with same title (all regex capture groups are concatenated)", default=r'(.+)\/')
    parser.add_argument('-g', '--groupmatchregex', help="Regex used to format benchmark name into chart categories (all regex capture groups are concatenated)")
    parser.add_argument('-b', '--legendformatregex', help="Regex used to format benchmark name for data legend (all regex capture groups are concatenated)", default=r'([^\/]+$)')
    parser.add_argument('-l', '--labelformatregex', help="Regex used to format file names for chart data labels (all regex capture groups are concatenated)", default=r'(?i)([^\/|^\\]+)\.json$')
    parser.add_argument('-i', '--htmlheaderfile', help="Inserted at the top of the html output, can be used to set global chart.js config. Default: charts_html_header.html")
    args = parser.parse_args()

    if args.directory is None:
        args.directory = os.getcwd()

    if args.htmlheaderfile is None:
        args.htmlheaderfile = str(os.path.dirname(os.path.realpath(sys.argv[0]))) + '/charts_html_header.html'
    return args

def try_regex_find(regex, string):
    regexmatch = re.findall(regex, string)

    # return unmodified string if no groups are found
    if len(regexmatch) == 0:
        return string

    return "".join(regexmatch[0])
    
def parse_benchmark_file(file, benchmarks, metric, filterregex):
    print('Parsing ' + file)
    
    with open(file) as json_file:
        data = json.load(json_file)

        # Parse date, remove ':' so python can parse UTC offset
        date = datetime.strptime( data['context']['date'].replace(':',''), '%Y-%m-%dT%H%M%S%z' )

        print('Found '+ str(len(data['benchmarks'])) +' benchmarks in json')

        for b in data['benchmarks']:
            benchname = b['name']

            # Check becnhmark name filter
            if re.search(filterregex, benchname) is None:
                continue

            if benchmarks.get(benchname) is None:
                benchmarks[benchname] = { 'filedate': {}, 'data': [] }
            
            # Can't represent duplicate bench names in chart, ignore them
            if benchmarks[benchname]['filedate'].get(str(file)) is not None:
                print('Ignoring duplicate benchmark "' + benchname + '" in: ' + str(file))
                continue

            benchmarks[benchname]['filedate'][str(file)] = date
            benchmarks[benchname]['data'].append(b[metric])

def main():
    # parse cmd args
    args = create_parser()
    
    # get list of files to parse
    files = []
    for entry in os.scandir(args.directory):
        if entry.path.lower().endswith(".json") and entry.is_file():
            files.append(entry)

    if len(files) == 0:
        print('Found 0 json files in dir: ' + args.directory)
        exit()
    
    # sort them in order of modification time (newest to oldest)
    # ensures we use the benchmark order from the most recent results
    files.sort(key=os.path.getmtime, reverse=True)
    
    # parse benchmark files
    benchmarks = {}
    for entry in files:
        try:
            parse_benchmark_file(entry.path, benchmarks, args.metric, args.filterregex)
        except:
            print('Benchmark json parse failed: ' + entry.path)
                
    # analyse benchmarks
    chartgroups = {}
    for benchmark in benchmarks:  
        sample_count = len(benchmarks[benchmark]['data'])
        print('found ' + str(sample_count) + ' benchmark records for benchmark ' + benchmark)
        
        # find group name
        groupname = ""
        if args.groupmatchregex is not None:
            regexmatch = re.findall(args.groupmatchregex, benchmark)
            if len(regexmatch) > 0:
                groupname = "".join(regexmatch[0])

        # add to group
        if chartgroups.get(groupname) is None:
            chartgroups[groupname] = {}

        charts = chartgroups[groupname]

        # format chart name
        chartname = try_regex_find(args.chartmatchregex, benchmark)

        if charts.get(chartname) is None:
            charts[chartname] = { 'benchmarks': [], 'filedate': {} }
        
        # Get list of all benchmark files in this chart before converting to chart data
        charts[chartname]['filedate'].update(benchmarks[benchmark]['filedate'])
        charts[chartname]['benchmarks'].append(benchmark)

    # html header
    htmloutput = """
        <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/materialize/1.0.0/css/materialize.min.css">
        <script src="https://cdnjs.cloudflare.com/ajax/libs/materialize/1.0.0/js/materialize.min.js"></script>
        <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.1.1/chart.min.js"></script>
        <script>var allcharts = []</script>\n"""
    
    with open(args.htmlheaderfile, 'r') as headerfile:
        htmloutput += headerfile.read()  

    htmloutput += '\n<ul class="collapsible">'

    # html template for chart, width/height aspect ratio is preserved
    htmlcharttemplate = '<canvas width="18" height="8"></canvas><script>allcharts.push(new Chart([...document.getElementsByTagName("canvas")].slice(-1)[0].getContext("2d"), %CHARTDATA%));</script>'

    # create collapsible html groups
    for group in chartgroups:
        grouphtml = ''
        chartsingroup = False     

        # html collapsible section for groups
        if group != '':
            grouphtml += '<li><div class="collapsible-header">' + group + '</div>'
            grouphtml += '<div class="collapsible-body">'

        # create chart html
        for chart in chartgroups[group]:
            # sort based on benchmark run datetime (oldest to newest)
            filedatesorted = sorted(chartgroups[group][chart]['filedate'].items(), key=operator.itemgetter(1))

            # only use N most recent results
            if int(args.maxchartentries) > 0:
                filedatesorted = filedatesorted[-int(args.maxchartentries):]

            # create data structure needed for Chart.js
            chartdata = {
                'type': args.charttype,
                'data': {
                    'labels': [try_regex_find(args.labelformatregex, x[0]) for x in filedatesorted],
                    'files': [x[0] for x in filedatesorted],
                    'dates': [str(x[1]) for x in filedatesorted],
                    'datasets': []
                },
                'options':{ 'plugins':{
                    'title':{ 
                        'text': chart
                    }
                }, 
                'scales':{ 'y': {
					'title': {
						'text': args.metric
					}
				}}}}

            for benchmark in chartgroups[group][chart]['benchmarks']:
                # chart.js dataset format
                chartdataset = {'label': try_regex_find(args.legendformatregex, benchmark), 'data': []}

                filedatekeys = list(benchmarks[benchmark]['filedate'].keys())
                founddata = False

                # find data from matching file if it exists
                for filedate in filedatesorted:
                    try:
                        idx = filedatekeys.index(filedate[0])
                        chartdataset['data'].append(benchmarks[benchmark]['data'][idx])
                        founddata = True
                    except:
                        # invalid data will be skipped in chart.js
                        chartdataset['data'].append('NaN')

                if founddata:
                    chartdata['data']['datasets'].append(chartdataset)

            # fill in chart data to html chart template
            if len(chartdata['data']['datasets']) > 0:
                charthtml = htmlcharttemplate
                charthtml = charthtml.replace("%CHARTDATA%", str(chartdata))
                
                grouphtml += charthtml
                chartsingroup = True
        
        if group != "":
            grouphtml += '</div></li>'

        # Only add the group html if it contains charts
        if chartsingroup:
            htmloutput += grouphtml

    # html footer, manages collapsible sections
    htmloutput += "</ul><script>M.AutoInit()</script>"

    # save html file
    with open(args.outputfile, 'w') as outfile:
        outfile.write(htmloutput)
        
if __name__ == '__main__':
    main()
