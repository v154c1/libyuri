#!/usr/bin/python3
# encoding: utf-8
'''
Module implementing yuri routing grammer
'''
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
    if 'ins' not in pstr.context:
        pstr.context['ins'] = set()
    if 'outs' not in pstr.context:
        pstr.context['outs'] = set()
    return pstr
# Define grammar

ID = pp.Word(pp.alphas + '/', pp.alphanums + '_/-')
NAME = ID | '*'
# missing default value syntax
SPEC = (ID + ':' + NAME).setParseAction(lambda s,
                                               l, t: s.context['ins'].add((t[0], t[2])))
OUT_SPEC = (ID + ':' + NAME).setParseAction(lambda s,
                                                   l, t: s.context['outs'].add((t[0], t[2])))
OUT_LIST = pp.Forward()
OUT_LIST << ((OUT_SPEC + ',' + OUT_LIST) | OUT_SPEC)
NULL = pp.CaselessLiteral('null')
BANG = pp.CaselessLiteral('bang')
BOOL = pp.CaselessLiteral('true') | pp.CaselessLiteral('false')
INT = pp.Word(pp.nums)
DOUBLE = (INT + pp.Literal('.') + INT) | (INT + '.')
STRING = '"' + pp.CharsNotIn('"') + '"'
# missing vector and dict
ARG = pp.Forward()
FARG = pp.Forward()
VECTOR = pp.nestedExpr('[', ']', FARG)
CONST = NULL ^ BANG ^ BOOL ^ INT ^ DOUBLE ^ STRING ^ VECTOR

FUNC = pp.Forward()

ARG << (SPEC | CONST | FUNC)

FARG << ((ARG + ',' + FARG) | ARG)
# .setParseAction(lambda s,l,t: parse_typer('func', (t[0])))
FUNC << (ID + pp.nestedExpr('(', ')', FARG))
EXPR = SPEC | FUNC
ROUTE = 'route' + pp.nestedExpr('(', ')', EXPR) + '->' + OUT_LIST


NUM = DOUBLE | INT


def parse_route(instr, ctx):
    '''
    Parses a single route
    '''
    pstring = ctx_string(instr, ctx)
    parsed = ROUTE.parseString(pstring).asList()
    return pstring.context, parsed

