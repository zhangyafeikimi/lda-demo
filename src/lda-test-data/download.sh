#! /bin/bash

wget https://archive.ics.uci.edu/ml/machine-learning-databases/bag-of-words/docword.nips.txt.gz
wget https://archive.ics.uci.edu/ml/machine-learning-databases/bag-of-words/vocab.nips.txt
gunzip docword.nips.txt.gz
python gen_uci.py docword.nips.txt nips-train
cp vocab.nips.txt nips-vocab

wget https://archive.ics.uci.edu/ml/machine-learning-databases/bag-of-words/docword.enron.txt.gz
wget https://archive.ics.uci.edu/ml/machine-learning-databases/bag-of-words/vocab.enron.txt
gunzip docword.enron.txt.gz
python gen_uci.py docword.enron.txt enron-train
cp vocab.enron.txt enron-vocab

wget https://archive.ics.uci.edu/ml/machine-learning-databases/bag-of-words/docword.nytimes.txt.gz
wget https://archive.ics.uci.edu/ml/machine-learning-databases/bag-of-words/vocab.nytimes.txt
gunzip docword.nytimes.txt.gz
python gen_uci.py docword.nytimes.txt nytimes-train
cp vocab.nytimes.txt nytimes-vocab

wget https://archive.ics.uci.edu/ml/machine-learning-databases/bag-of-words/docword.pubmed.txt.gz
wget https://archive.ics.uci.edu/ml/machine-learning-databases/bag-of-words/vocab.pubmed.txt
gunzip docword.pubmed.txt.gz
python gen_uci.py docword.pubmed.txt pubmed-train
cp vocab.pubmed.txt pubmed-vocab
