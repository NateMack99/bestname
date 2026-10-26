import datasets
import argparse
from itertools import combinations
import networkx as nx
import string

ALLOWED_CHARS = minecraft_username_chars = list(
    string.ascii_letters + string.digits + "_"
)

print(ALLOWED_CHARS)

# ds = datasets.load_dataset("mudkipdev/minecraft-usernames")
# ds["train"].to_csv("usernames.csv")
# head = ds["train"][:10000]["text"]
# with open("usernames_head.csv", "w", encoding="utf-8") as file:
#     for s in head:
#         file.write(f"{s}\n")
names = []
with open("usernames.csv", "r", encoding="utf-8") as file:
    for name in file:
        names.append(name.strip())

def filter_by_common_index(iters : int, strs: list[str]):
    indexed_dict = dict.fromkeys(ALLOWED_CHARS)
    for k, _ in indexed_dict.items():
        indexed_dict[k] = []
    for i in range(iters):
        for s in strs:
            for k in indexed_dict.keys():
                if k in s.lower():
                    indexed_dict[k].append(s)
        f_dict = filter_percentile(indexed_dict, 1/8**i)
        strs = list(set([
            s
            for _, s_list in f_dict.items()
            for s in s_list
        ]))
        indexed_dict = {
            f"{k}{c}" : []
            for k, _ in f_dict.items()
            for c in ALLOWED_CHARS
        }
    return strs

def filter_percentile(d : dict, top_n_percent: float):
    l = [(k, v) for k, v in d.items()]
    l.sort(key = lambda entry : len(entry[1]), reverse = True)
    num_to_keep = int(top_n_percent * len(l) + 0.999)
    print(f"top 10: {[p[0] for p in l[:10]]}")
    return {p[0] : p[1] for p in l[:num_to_keep]}

def write_to_file(name: str, list: list[str]):
    with open(name, "w", encoding="utf-8") as file:
        for s in list:
            file.write(f"{s}\n")

for i in range(3, 6):
    filtered_names = filter_by_common_index(i, names)
    write_to_file(f"usernames_filtered_{i}.csv", filtered_names)

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
