#!/usr/bin/python3
# encoding: utf-8
'''
Processes yuri xml and outputs dot representation of the graph
'''

import sys
import xml.etree.ElementTree as ET
from hashlib import md5
import argparse
from grammar import parse_route

def dbg(*args, **kwargs):
    '''
    Prints text to stderr
    '''
    print(*args, file=sys.stderr, **kwargs)


def process_events(root):
    '''
    Processes event links
    '''
    dot = ''
    for events in root.findall('event'):
        lines = events.text.split(';')
        for line in lines:
            line = line.strip()
            if not line:
                continue
            try:
                # print ('Parsing: ' + str(line))
                ctx, res = parse_route(line, {})
                # print('ctx: ' + str(ctx))
                if not ctx['ins'] or not ctx['outs']:
                    continue
                # simple route has to have only a single input and contain SPEC
                # as only param (in res[1]) to route
                if len(ctx['ins']) == 1 and len(res) > 1 and len(res[1]) > 1 and res[1][1] == ':':
                    # simple route(s)
                    in_node = ctx['ins'].pop()
                    for out_node in ctx['outs']:
                        dot = dot + '%s -> %s [label="%s -> %s", style="dotted"];\n' % (
                            in_node[0],
                            out_node[0],
                            in_node[1],
                            out_node[1])
                else:
                    txt = line[5:].split('->')[0].strip()[1:-1].strip()
                    name = 'X' + md5(txt.encode('utf8')).hexdigest()
                    dot = dot + \
                        '%s [label="%s",style="filled"];\n' % (name, txt)
                    for in_node in ctx['ins']:
                        dot = dot + '%s -> %s [label="%s", style="dotted"];\n' % (
                            in_node[0],
                            name,
                            in_node[1])
                    for out_node in ctx['outs']:
                        dot = dot + '%s -> %s [label="%s", style="dotted"];\n' % (
                            name,
                            out_node[0],
                            out_node[1])
            except Exception as exc:
                dbg('Failed to process line "%s": %s' % (line, str(exc)))
                raise
    return dot


def link_info(attrib, slots):
    '''
    Returns string describing a link
    '''
    def strip_slot(spec):
        '''
        Strips slot number from source/target specification
        '''
        return ':'.join(spec.split(':')[:-1])

    def get_slot(spec):
        '''
        Strips slot number from source/target specification
        '''
        return ':'.join(spec.split(':')[-1])
    if slots:
        return '%s -> %s [label="%s\\n(%s)",headlabel="%s",taillabel="%s"];\n' % (
            strip_slot(attrib.get('source')),
            strip_slot(attrib.get('target')),
            attrib.get('name'),
            attrib.get('class'),
            get_slot(attrib.get('target')),
            get_slot(attrib.get('source')))
    return '%s -> %s [label="%s\\n(%s)"];\n' % (
        strip_slot(attrib.get('source')),
        strip_slot(attrib.get('target')),
        attrib.get('name'),
        attrib.get('class'))


def process_file(fname, events=True, slots=False):
    '''
    returns a dot string
    '''
    tree_it = ET.iterparse(fname)
    for _, elem in tree_it:
        if '}' in elem.tag:
            elem.tag = elem.tag.split('}', 1)[1]  # strip all namespaces
    root = tree_it.root

    if root.tag != 'app':
        dbg('Not a valid yuri config')
        sys.exit(1)

    dot = '%s {\n' % root.attrib.get('name', 'yuri')

    for node in root.findall('node'):

        name = node.attrib.get('name')
        dot = dot + \
            '%s [label="%s\\n(%s)"];\n' % (
                name, name, node.attrib.get('class'))

    for link in root.findall('link'):
        dot = dot + link_info(link.attrib, slots)

    if events:
        dot = dot + process_events(root)

    dot = dot + '}\n'
    return dot

if __name__ == '__main__':

    # EVENTS = sys.argv[2] != '0' if len(sys.argv) > 2 else True
    PARSER = argparse.ArgumentParser(
        description='Creates DOT graph representation of yuri configuration XML')
    PARSER.add_argument('--slots', '-s', help='Include slot numbers in the graph',
                        dest='slots', default=False, action='store_true')
    PARSER.add_argument('--no-events', '-n', help='Do not output edges and nodes fdor event routes',
                        dest='no_events', default=False, action='store_true')
    PARSER.add_argument('cfg_file', help='Yuri XML config')
    ARGS = PARSER.parse_args()

    GRAPH = 'digraph ' + process_file(ARGS.cfg_file, not ARGS.no_events, ARGS.slots)
    print(GRAPH)
