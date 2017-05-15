#!/usr/bin/python3
# encoding: utf-8
'''
Script creates simple XML from a command line (compatible with yuri_simple)
'''
import argparse
import pyparsing as pp


class PString(str):
    '''
    Class adding a context to ta string
    '''
    context = None

    def expandtabs(self, tabs=8):
        ret = type(self)(super(PString, self).expandtabs(tabs))
        ret.context = self.context
        return ret


def ctx_string(instr, ctx):
    '''
    Creates a string with a context
    '''
    pstr = PString(instr)
    pstr.context = ctx
    if 'params' not in pstr.context:
        pstr.context['params'] = []
    return pstr

# Define grammar
#
# grammar si quite simple: it's just a long chain of node specifications
# Each node is either just ID or ID[<params>],
# where <params> is comma separated list of name=value
#

ID = pp.Word(pp.alphas + '/', pp.alphanums + '_/-')
NAME = pp.Word(pp.alphas + '/', pp.alphanums + '_/-')
NAME.addParseAction(lambda s, l, t: s.context.update({'name': t[0]}))
VALUE = pp.CharsNotIn(',"] ') ^ pp.QuotedString('"', '\\')
PARAM = ID + '=' + VALUE
# PARAM.addParseAction(add_param)
PARAM.addParseAction(lambda s, l, t: s.context['params'].append((t[0], t[2])))
PARAMS = pp.delimitedList(PARAM)
PARAM_LIST = pp.nestedExpr('[', ']', PARAMS)
NODE = NAME ^ (NAME + PARAM_LIST)
NODE.addParseAction(lambda s, l, t: s.context.update({'type': 'node'}))
PIPE = ('-p' + NAME) ^ ('-p' + NAME + PARAM_LIST)
PIPE.addParseAction(lambda s, l, t: s.context.update({'type': 'pipe'}))

GRAMMAR = PIPE ^ NODE


def parse_node(instr, ctx):
    '''
    Parses a single route
    '''
    pstring = ctx_string(instr, ctx)
    parsed = GRAMMAR.parseString(pstring).asList()
    return pstring.context, parsed


def generate_tag(name, args, content, single_line=False):
    '''
    Generates generic XML tag
    '''
    sep = '' if single_line else '\n'
    return u'<%s' % name + u''.join((u' %s="%s"' % (str(a[0]), str(a[1])) for a in args))\
           + (u'/>' if not content else u'>' +
              sep + content + sep + u'</%s>' % name)


def generate_params(params):
    '''
    Generates <parameter> tags
    '''
    return u'\n'.join(('\t' + generate_tag('parameter',
                                           [('name', p[0])],
                                           str(p[1]),
                                           single_line=True) for p in params))

def generate_xml_node(name, args, params):
    '''
    Generates yuri xml node with parameters
    '''
    return generate_tag(name, args, generate_params(params)) + '\n\n'


if __name__ == '__main__':
    PARSER = argparse.ArgumentParser(
        description='Creates XML representation of yuri graph')
    PARSER.add_argument(
        '--name', '-n', help='Graph name', dest='name', default='yuri_graph')
    ARGS, OTHER = PARSER.parse_known_args()

    XML = u'<?xml version="1.0" ?>\n<app name="%s">\n' % ARGS.name

    PIPES = u''
    PIPE_PARAMS = {'name': 'single', 'params': []}
    LAST_NODE = u''
    for idx, node in enumerate((parse_node(node, {})[0] for node in OTHER)):
        if node['type'] == 'pipe':
            PIPE_PARAMS = node
        else:
            NAME = '%s_%d' % (node['name'], idx)
            XML += generate_xml_node('node',
                                     [('class', node['name']), ('name', NAME)], node['params'])
            if LAST_NODE:
                PIPES += generate_xml_node('link', [
                    ('class', PIPE_PARAMS['name']),
                    ('name', '%s_%d' % (PIPE_PARAMS['name'], idx)),
                    ('source', LAST_NODE + ':0'),
                    ('target', NAME + ':0')],
                                           PIPE_PARAMS['params'])
            LAST_NODE = NAME

    XML += '\n' + PIPES + '\n</app>\n'
    print(XML)
