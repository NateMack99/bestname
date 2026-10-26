from datasets import load_dataset
import argparse
from itertools import combinations
import networkx as nx

ds = load_dataset("mudkipdev/minecraft-usernames")
# ds["train"].to_csv("usernames.csv")


head = ds["train"][:10000]["text"]
with open("usernames_head.csv", "w", encoding="utf-8") as file:
    for s in head:
        file.write(f"{s}\n")


def related(a, b):
    """Substring, one substitution, or one insertion/deletion anywhere."""
    if a == b:
        return False  # No self-loops.
    if a in b or b in a:
        return True
    if len(a) == len(b):
        return sum(x != y for x, y in zip(a, b)) == 1
    if len(a) > len(b):
        a, b = b, a
    if len(b) != len(a) + 1:
        return False
    # Skip the first mismatching character in the longer string.
    i = 0
    while i < len(a) and a[i] == b[i]:
        i += 1
    return a[i:] == b[i + 1:]


def build_graph(words):
    graph = nx.Graph()
    graph.add_nodes_from(words)
    for a, b in combinations(graph.nodes, 2):
        if related(a, b):
            graph.add_edge(a, b)
    return graph
