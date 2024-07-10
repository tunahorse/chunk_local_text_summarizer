Work in progress, trying to slove a problem with using small CW llm's and large token inputs. 



The text summarizer uses the TextRank algorithm. 

The input text is split into sentences.
Each sentence is treated as a node in a graph.
Sentences are compared to each other to calculate similarity scores.
The TextRank algorithm is applied to rank sentences based on their importance.
The top-ranking sentences are selected to form the summary.
The selected sentences are arranged in their original order to maintain coherence.
The summary is written in an output file.
