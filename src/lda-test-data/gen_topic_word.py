#! /usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Yafei Zhang (zhangyafeikimi@gmail.com)
#
# visualize doc: top-topic results
#


import sys
from vocabulary import Vocabulary


def load_topic_word_prob_file(filename):
    fp = open(filename, 'r')
    topic_prob_list_map = {}
    topic = 1
    for line in fp.readlines():
        prob_list = []
        _id = 1
        for p in line.split():
            prob_list.append((_id, p))
            _id += 1
        topic_prob_list_map[topic] = prob_list
        topic += 1
    fp.close()
    return topic_prob_list_map


def save_topic_word_prob_readable(filename,
                                  topic_prob_list_map,
                                  vocab,
                                  threshold=None):
    if threshold is None:
        threshold = 1e-6
    fp = open(filename, 'w')
    for topic in topic_prob_list_map.keys():
        fp.write('topic %d\n' % topic)
        prob_list = topic_prob_list_map[topic]
        prob_list = [(_id, float(p)) for _id, p in prob_list
                     if float(p) >= threshold]
        prob_list.sort(key=lambda tup: tup[1], reverse=True)
        for (_id, p) in prob_list:
            fp.write('%24.24s:%8.8f\n' % (vocab.get_token_from_id(_id), p))
        fp.write('--------------------\n')
    fp.close()


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print >> sys.stderr, '%s [vocabulary] [xxx-topic-word]' % sys.argv[0]
        sys.exit(1)

    vocab = Vocabulary()
    vocab.load(sys.argv[1])
    topic_prob_list_map = load_topic_word_prob_file(sys.argv[2])

    save_topic_word_prob_readable(sys.argv[2] + '-readable',
                                  topic_prob_list_map,
                                  vocab)
    save_topic_word_prob_readable(sys.argv[2] + '-readable.0.005',
                                  topic_prob_list_map,
                                  vocab,
                                  threshold=0.005)
