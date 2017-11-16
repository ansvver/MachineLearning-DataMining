#!/bin/bash

tar xvzf cooking.stackexchange.tar.gz
head -n 12404 cooking.stackexchange.txt > cooking.train
tail -n 3000 cooking.stackexchange.txt > cooking.valid
./fasttext supervised -input cooking.train -output model_cooking

# ./fasttext predict model_cooking.bin -
# ./fasttext predict model_cooking.bin - 5

./fasttext test model_cooking.bin cooking.valid
./fasttext test model_cooking.bin cooking.valid 5

# preprocessing the data ;
cat cooking.stackexchange.txt | sed -e "s/\([\.\!\?,'\/\(\)]\)/ \1 /g" | tr "[:upper:]" "[:lower:]" > cooking.preprocessed.txt
head -n 12404 cooking.preprocessed.txt > cooking.train
tail -n 3000 cooking.preprocessed.txt > cooking.valid
./fasttext supervised -input cooking.train -output model_cooking
./fasttext test model_cooking.bin cooking.valid

# changing the number of epochs (using the option -epoch, standard range [5 - 50]) ;
./fasttext supervised -input cooking.train -output model_cooking -epoch 25
./fasttext test model_cooking.bin cooking.valid

# changing the learning rate (using the option -lr, standard range [0.1 - 1.0]) ;
./fasttext supervised -input cooking.train -output model_cooking -lr 1.0  
./fasttext test model_cooking.bin cooking.valid

./fasttext supervised -input cooking.train -output model_cooking -lr 1.0 -epoch 25
./fasttext test model_cooking.bin cooking.valid

# using word n-grams (using the option -wordNgrams, standard range [1 - 5]).
./fasttext supervised -input cooking.train -output model_cooking -lr 1.0 -epoch 25 -wordNgrams 2
./fasttext test model_cooking.bin cooking.valid

# use the hierarchical softmax, instead of the regular softmax
./fasttext supervised -input cooking.train -output model_cooking -lr 1.0 -epoch 25 -wordNgrams 2 -bucket 200000 -dim 50 -loss hs
