#!/usr/bin/python3
import sys
import os
import io
import pycurl
import json
import logging
import colorama
import argparse
import time
import re
from colorama import Fore, Style
from datetime import datetime
from io import BytesIO

jenkins_projects = [
    "98520_NA51055_DUAL_EVB_CARDV_SDK_1_40_1_XML",
    "98520_NA51055_DUAL_EVB_CARDV_SDK_1_40_2_XML",
]


class JenkinsError(Exception):
    pass


class Jenkins(object):
    def __init__(self):
        super(Jenkins, self).__init__()
        self.logger = logging.getLogger()
        self.logger.setLevel(logging.DEBUG)

    def get_curl_json(self, uri):
        # print(uri)
        n_retry = 3
        data = BytesIO()

        def write_data(buf):
            data.write(buf)

        while(n_retry):
            jenkins = pycurl.Curl()
            jenkins.setopt(pycurl.URL, uri)
            jenkins.setopt(pycurl.WRITEFUNCTION, write_data)
            try:
                jenkins.perform()
            except pycurl.error:
                self.logger.error("pycurl.error, retry again")
                n_retry -= 1
                jenkins.close()
                continue
            if jenkins.getinfo(pycurl.HTTP_CODE) != 200:
                self.logger.error("http error status = {}".format(jenkins.getinfo(pycurl.HTTP_CODE)))
                return None
            jenkins.close()
            break

        json_data = json.loads(data.getvalue().decode("utf-8"))
        if json_data.get("actions") is None:
            self.logger.error(json_data)
            return None
        return json_data

    def get_summary(self, jenkins_prj):
        summary = list()
        uri_summary = "http://oajenkins1.novatek.com.tw/job/{}/api/json".format(jenkins_prj)
        json_summary = self.get_curl_json(uri_summary)
        if json_summary is None:
            return summary
        for desc in json_summary["builds"]:
            item = self.get_curl_json(desc["url"]+"api/json")
            summary.append(item)
        return summary

def parse_args(argv):
    parser = argparse.ArgumentParser(description='jenkins report')
    parser.add_argument('-c', '--contiguous', action='store_true', help='while 1 to check jenkins report')
    parser.add_argument('-m', '--menu', action='store_true', help='pop a menu to list nearly 50 builds status')
    args = parser.parse_args()
    return args

def _check_valid(line):
    logger = logging.getLogger()
    re_valids = [
        re.compile(r"\d+\s+\d+"), # invalid: 5 6 (use space as separator)
        re.compile(r"[^\s^\d^,]"), # invalid: t, $, (invalid symbol)
    ]
    for re_valid in re_valids:
        match = re_valid.search(line)
        if match is not None:
            logger.error("invalid input => {}. Retry again.".format(match.group(0)))
            return False
    return True

def menu_select(menu):
    print("=============================================================")
    for idx, item in enumerate(menu):
        print("{:2d}: {}".format(idx, item["text"]))
    print("=============================================================")
    print("")
    print("item separtor is comma ',' rather than space ' '")
    print("e.g.,")
    print("  0-8        [run items from 0 to 8]")
    print("  9          [only run item 9]")
    print("  3, 5, 9-12 [run items only 3, 5, 9, 10, 11, 12]")
    print("  q          [quit]")
    print("")
    valid = False
    while not valid:
        print("Input: ", end="")
        item_str = input().split(',')
        if item_str[0] == 'q':
            return []
        elif item_str[0] == '':
            continue
        item_list = list()

        valid = True
        for item in item_str:
            if item == "":
                continue
            elif "-" not in item:
                valid = _check_valid(item)
                if not valid:
                    break
                item_list.append(int(item))
            else:
                two_item = item.split('-')
                item_from = int(two_item[0])
                item_to = int(two_item[1])
                for i in range(item_from, item_to+1):
                    item_list.append(i)

        if not valid:
            continue

        # check out of range
        valid = True
        for item in item_list:
            if item >= len(menu):
                print("item: {} is out of range. Retry again.".format(item))
                valid = False
                break
    return item_list

def make_report(item):
    report = ""
    is_building  = item["building"]
    dt = datetime.fromtimestamp(item["timestamp"]/1000)
    now_time = datetime.now()
    est_time = datetime.fromtimestamp((item["timestamp"] + item["estimatedDuration"])/1000)
    end_time = datetime.fromtimestamp((item["timestamp"] + item["duration"])/1000)
    if est_time > now_time:
        rest_min = int(((est_time-now_time).seconds) / 60 * (-1)) - 1
    else:
        rest_min = int(((now_time - est_time).seconds) / 60) + 1
    est_second = item["estimatedDuration"]/1000
    percent = int(((now_time - dt).seconds) / est_second * 100)
    result = item["result"]
    number = item["number"]
    email = item["actions"][0]["parameters"][2]["value"]
    hash_id = (item["actions"][0]["parameters"][6]["value"])[0:11]
    repo_name = item["actions"][0]["parameters"][11]["value"]
    if is_building:
        report += "[{:4d}][{:4d}m|{:3d}%] {:02d}-{:02d} {:02d}:{:02d}-??:??  {:11s}  {:20s} {:30s}".format(number, rest_min, percent, dt.month, dt.day, dt.hour, dt.minute, hash_id, repo_name, email)
    else:
        if result == "SUCCESS":
            report += Fore.GREEN + "[{:4d}][   {}] {:02d}-{:02d} {:02d}:{:02d}-{:02d}:{:02d}  {:11s}  {:20s} {:30s}".format(number, result, dt.month, dt.day, dt.hour, dt.minute, end_time.hour, end_time.minute, hash_id, repo_name, email) + Style.RESET_ALL
        elif result == "FAILURE":
            report += Fore.RED + Style.BRIGHT + "[{:4d}][   {}] {:02d}-{:02d}-{:02d}:{:02d}-{:02d}:{:02d}  {:11s}  {:20s} {:30s}".format(number, result, dt.month, dt.day, dt.hour, dt.minute, end_time.hour, end_time.minute, hash_id, repo_name, email) + Style.RESET_ALL
        else:
            report += Fore.YELLOW + "[{:4d}][   {}] {:02d}-{:02d} {:02d}:{:02d}-{:02d}:{:02d}  {:11s}  {:20s} {:30s}".format(number, result, dt.month, dt.day, dt.hour, dt.minute, end_time.hour, end_time.minute, hash_id, repo_name, email) + Style.RESET_ALL
    return report

def run_one(jenkins):
    report =""
    for project in jenkins_projects:
        title = "===== [{}] ".format(project)
        report += title
        for i in range(80-len(title)):
            report += "="
        report += "\n"
        summary = jenkins.get_summary(project)
        for item in summary:
            report += make_report(item) + "\n"
            if item["result"] == "SUCCESS":
                break
        report += "================================================================================\n"
    os.system('cls' if os.name == 'nt' else 'clear')
    print(report)

def get_log_from_url(url):
    n_retry = 3
    data = BytesIO()
    def write_data(buf):
        data.write(buf)
    while(n_retry):
            curl = pycurl.Curl()
            curl.setopt(pycurl.URL, url)
            curl.setopt(pycurl.WRITEFUNCTION, write_data)
            try:
                curl.perform()
            except pycurl.error:
                logging.error("pycurl.error, retry again")
                n_retry -= 1
                curl.close()
                continue
            if curl.getinfo(pycurl.HTTP_CODE) != 200:
                logging.error("http error status = {}".format(curl.getinfo(pycurl.HTTP_CODE)))
                return None
            curl.close()
            break
    text = io.StringIO(data.getvalue().decode("utf-8"))
    for line in text:
        line = line.strip()
        if "Log stdout file: " in line:
            break
    if len(line) == 0:
        return None
    line = line.replace('Log stdout file: ','')
    line = line.replace('/','\\')
    return line

def detial_one(item, project):
    url = item["url"]
    repo_name = item["actions"][0]["parameters"][11]["value"]
    hash_id = (item["actions"][0]["parameters"][6]["value"])
    stdout = get_log_from_url(url+"console")
    stdout = "G:\\BinaryCode\\{}\\{}".format(project, stdout)
    bin = stdout.replace("-stdout.log","")
    bin = bin.replace("log-","bin-")
    print("[console] {}console".format(url))
    print("[git] http://git.novatek.com.tw/projects/IVOT_BSP/repos/{}/commits/{}".format(repo_name, hash_id))
    print("[stdout] {}".format(stdout))
    print("[stderr] {}".format(stdout.replace("-stdout.log","-stderr.log")))
    print("[bin] {}".format(bin))
    return

def list_one(jenkins, project):
    i = 0
    menu = []
    report ="http://oajenkins1.novatek.com.tw/job/{}/\n".format(project)
    summary = jenkins.get_summary(project)
    for item in summary:
        # report += make_report(item)
        menu.append({"text": make_report(item), "func":detial_one})
        i += 1
        if i > 50:
            break
    #print(report)
    item_list = menu_select(menu)
    if len(item_list) == 0:
        return
    for idx in item_list:
        msg = "Running Item[{}]: {}".format(idx,menu[idx]["text"])
        print(msg)
        menu[idx]["func"](summary[idx], project)

def menu_mode(jenkins):
    menu = []
    for project in jenkins_projects:
        menu.append({"text": project, "func":list_one})
    item_list = menu_select(menu)
    if len(item_list) == 0:
        return
    for idx in item_list:
        msg = "Running Item[{}]: {}".format(idx,menu[idx]["text"])
        print(msg)
        menu[idx]["func"](jenkins, jenkins_projects[idx])

def main(argv):
    colorama.init()
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    jenkins = Jenkins()
    args = parse_args(argv)
    if args.contiguous:
        print("use ctrl+c to stop.")
        while 1:
            run_one(jenkins)
            time.sleep(60)
    if args.menu:
        menu_mode(jenkins)
        return 0
    run_one(jenkins)
    return 0

if __name__ == '__main__':
    main(sys.argv)
