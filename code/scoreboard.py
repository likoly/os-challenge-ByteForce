class Data:
    def __init__(self):
        self.results = {
            'array': [],
            'heap': [],
            'linked': [],
            'none': []
        }
        self.averages = {
            'array': 0,
            'heap': 0,
            'linked': 0,
            'none': 0
        }

    def calculate_averages(self):
        # Calculate the average for each category in `values` and store in `averages`
        for key, runs in self.results.items():
            if runs:
                self.averages[key] = sum(runs) / len(runs)
            else:
                self.averages[key] = 0  # Handle empty lists to avoid division by zero
    # def print_averages(self):
    #     for key,average in self.averages.items():
            
    def __iter__(self):
        # Returns an iterator over all categories in `values`
        return iter(self.results.items())
import pandas as pd

# Load the CSV file into a DataFrame
df = pd.read_csv('/home/swagger/code/server/master/code/results.csv')

# Initialize Data object (assuming the Data class is already defined)
algorithms = Data()

for index, row in df.iterrows():
    algorithm_name = row['Algorithm']
    
    # Filter out any NaN values or empty strings from the run columns
    runs = [int(run) for run in row[1:].tolist() if pd.notna(run) and str(run).strip()]

    # Only add non-empty lists to algorithms.results
    if runs:
        algorithms.results[algorithm_name] = runs

algorithms.calculate_averages()

sorted_averages = sorted(algorithms.averages.items(), key=lambda x: x[1])

# Format the sorted list as "number, algorithm, run"
formatted_list = [f"{index + 1}, {name}, {average}" for index, (name, average) in enumerate(sorted_averages)]

# Output the formatted list
for item in formatted_list:
    print(item)