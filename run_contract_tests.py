import sys
import os
import subprocess
import requests
import hashlib


def parse_and_run_feature_file():
    FEATURE_FILE = sys.argv[1]
    collect_result = False
    test_suite_failed = False
    
    with open(FEATURE_FILE, 'r') as fd:
        feature_file = fd.read().split("\n")

    for line in feature_file:
        if "@contract=" in line:
            _, contract_name = line.split("=")
            res = requests.get(f"http://localhost:8090/init", params={"__contract__script__": contract_name})
            assert res
            collect_result = True

        if "Scenario:" in line:
            _, scenario = line.split(":")
            scenario = scenario.strip()
            
            os.system("./steps &")
            print(f"RUNNING SCENARIO {scenario}")

            res = subprocess.run(["cucumber", "features", "-n", scenario, "-f", "junit", "-o", f'results/{hashlib.sha256(scenario.encode()).hexdigest()}'])
            
            if res.returncode:
                test_suite_failed = True
        
        if collect_result:
            res = requests.get(f"http://localhost:8090/expect")
            assert res
            mock_server_response_data = res.json()
            assert not mock_server_response_data["expectations"]["failed"]
            collect_result = False
    
    if test_suite_failed:
        exit(1)

if __name__ == "__main__":
    parse_and_run_feature_file()    
