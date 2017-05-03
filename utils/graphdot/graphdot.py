#!/usr/bin/python3
# encoding: utf-8
'''
Processes yuri xml and outputs dot representation of the graph
'''

import sys
import xml.etree.ElementTree as ET

if len(sys.argv) < 2:
    print('Usage: %s <yuri.xml>' % sys.argv[0])
    sys.exit(1)

def process_file(fname):
    '''
    returns a dot string
    '''
    tree_it = ET.iterparse(fname)
    for _, el in tree_it:
        if '}' in el.tag:
            el.tag = el.tag.split('}', 1)[1]  # strip all namespaces
    root = tree_it.root

    if root.tag != 'app':
        print('Not a valid yuri config')
        sys.exit(1)

    dot = '%s {\n' % root.attrib.get('name', 'yuri')


    def strip_slot(spec):
        '''
        Strips slot number from source/target specification
        '''
        return ':'.join(spec.split(':')[:-1])

    for node in root.findall('node'):

        name = node.attrib.get('name')
        dot = dot + \
            '%s [label="%s\\n(%s)"];\n' % (name, name, node.attrib.get('class'))

    for link in root.findall('link'):
        dot = dot + '%s -> %s [label="%s\n(%s)"];\n' % (
            strip_slot(link.attrib.get('source')),
            strip_slot(link.attrib.get('target')),
            link.attrib.get('name'),
            link.attrib.get('class'))

    dot = dot + '}\n'
    return dot

DOT = 'digraph ' + process_file(sys.argv[1])

print (DOT)
