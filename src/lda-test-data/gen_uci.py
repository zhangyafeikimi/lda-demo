#! /usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Yafei Zhang (zhangyafeikimi@gmail.com)
#
# generate UCI training corpus
#

import sys

if __name__ == '__main__':
    if len(sys.argv) <= 2:
        print >> sys.stderr, '%s [docword] [output]' % sys.argv[0]
        sys.exit(1)

    infile = open(sys.argv[1], 'r')
    outfile = open(sys.argv[2], 'w')

    doc = infile.readline().strip('\r\n')
    vocab_word = infile.readline().strip('\r\n')
    total_word = infile.readline().strip('\r\n')
    print '%s documents' % doc
    print '%s words in vocabulary' % vocab_word
    print '%s words in all documents' % total_word

    prev_doc_id = None
    for line in infile:
        fields = line.strip().split(' ')
        doc_id = fields[0]
        word_id = fields[1]
        word_count = int(fields[2])

        if prev_doc_id is None:
            prev_doc_id = doc_id
            # outfile.write(doc_id)
        elif prev_doc_id != doc_id:
            prev_doc_id = doc_id
            outfile.write('\n')
            # outfile.write(doc_id)
        else:
            pass
        for i in range(0, word_count):
            outfile.write(' ' + word_id)

    outfile.write('\n')
