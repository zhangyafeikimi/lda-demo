#! /usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Yafei Zhang (zhangyafeikimi@gmail.com)
#
# generate yahoo training corpus
#


import os
import random
import sys
from vocabulary import Vocabulary
from porter_stemming import PorterStemmer


def stem_file(filename, vocab, stop_word=None):
    p = PorterStemmer()
    article = []
    infile = open(filename, 'r')
    while 1:
        word = ''
        line = infile.readline()
        if line == '':
            break
        for c in line:
            if c.isalpha():
                word += c.lower()
            else:
                if word:
                    stemmed_word = p.stem(word, 0, len(word) - 1)
                    word = ''
                    if stemmed_word is None or len(stemmed_word) == 0:
                        continue
                    if stop_word is not None and stop_word.get_id_from_token(
                            stemmed_word) != -1:
                        continue

                    vocab.add_token(stemmed_word)
                    article.append(stemmed_word)
    infile.close()
    # random.shuffle(article)
    return article


def collect_files(file_list, _dir):
    if os.path.isdir(_dir):
        for _d in os.listdir(_dir):
            collect_files(file_list, os.path.join(_dir, _d))
    elif os.path.isfile(_dir):
        file_list.append(_dir)


if __name__ == '__main__':
    if len(sys.argv) <= 3:
        print >> sys.stderr, '%s [stop word file] [output name] ' \
                             '[doc file] ...' % sys.argv[0]
        sys.exit(1)

    file_list = []
    for _dir in sys.argv[3:]:
        collect_files(file_list, _dir)

    stop_word = Vocabulary()
    stop_word.load(sys.argv[1])
    vocab = Vocabulary()
    articles = []

    for filename in file_list:
        article = stem_file(filename, vocab, stop_word)
        articles.append(article)
    # random.shuffle(articles)

    vocab.sort()
    vocab.save(sys.argv[2] + '-vocab')

    fp = open(sys.argv[2] + '-train', 'w')
    for article in articles:
        sb = ''
        for word in article:
            sb += '%d ' % vocab.get_id_from_token(word)
        sb = sb.rstrip()
        fp.write(sb)
        fp.write('\n')
    fp.close()
