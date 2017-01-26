#! /usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Yafei Zhang (zhangyafeikimi@gmail.com)
#


class Vocabulary:
    def __init__(self):
        self.id_token = {}
        self.token_id = None

    def clear(self):
        self.id_token = {}

    def add_token(self, token):
        self.id_token[token] = 0

    def sort(self):
        self.token_id = {}
        keys = self.id_token.keys()
        keys.sort()
        i = 0
        for k in keys:
            self.id_token[k] = i
            self.token_id[i] = k
            i += 1

    def get_id_from_token(self, token):
        return self.id_token.get(token, -1)

    def get_token_from_id(self, _id):
        return self.token_id.get(_id)

    def save(self, filename):
        fp = open(filename, 'w')
        keys = self.id_token.keys()
        keys.sort()
        for k in keys:
            fp.write(k)
            fp.write('\n')
        fp.close()

    def load(self, filename):
        self.clear()
        fp = open(filename, 'r')
        for k in fp.readlines():
            if len(k) == 1:
                continue
            self.add_token(k[:-1])
        fp.close()
        self.sort()
