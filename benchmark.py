import argparse
import subprocess
import matplotlib.pyplot as plt
import re

def run_program(program, threads, size):
    command = [program]
    if threads is not None:
        command.extend(["-t", str(threads)])
    command.extend(["-n", str(size)])
    result = subprocess.run(command, capture_output=True, text=True)
    match = re.search(r'Done in (\d+\.\d+) seconds.', result.stdout)
    if match:
        return float(match.group(1))


def benchmark(threads, size_min, size_max, ecart):
    sizes = list(range(size_min, size_max + 1, ecart))
    sharing_times = []
    stealing_times = []

    for size in sizes:
        sharing_time = run_program("./bin/sharing", threads, size)
        stealing_time = run_program("./bin/stealing", threads, size)
        sharing_times.append(sharing_time)
        stealing_times.append(stealing_time)
        print(f"Nombre d'éléments : {size} | sharing : {sharing_time} secondes, stealing : {stealing_time} secondes")

    return sizes, sharing_times, stealing_times

def plot_results(sizes, sharing_times, stealing_times, threads):
    plt.plot(sizes, sharing_times, label="Sharing")
    plt.plot(sizes, stealing_times, label="Stealing")
    plt.xlabel("Tailles")
    plt.ylabel("Temps")
    plt.title(f"Execution à {threads} threads")
    plt.savefig(f"benchmark_{threads}_threads.png")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", type=int, default=None)
    parser.add_argument("-n", type=int, default=10 * 1024 * 1024)
    parser.add_argument("-N", type=int, default=100 * 1024 * 1024)
    parser.add_argument("-e", type=int, default=10 * 1024 * 1024)
    args = parser.parse_args()

    sizes, sharing_times, stealing_times = benchmark(args.t, args.n, args.N, args.e)
    plot_results(sizes, sharing_times, stealing_times, args.t)
