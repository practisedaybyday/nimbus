#!/usr/bin/env python

# Author: Omid Mashayekhi <omidm@stanford.edu>

import boto.ec2
import time

import config

def run_instances(location, ami, num, key, sg, pg, it):

  ec2 = boto.ec2.connect_to_region(location)

  ec2.run_instances(
      ami,
      min_count = num,
      max_count = num,
      key_name = key,
      security_groups = [sg],
      placement_group = pg,
      instance_type = it)


def wait_for_instances_to_start(location, num):

  ec2 = boto.ec2.connect_to_region(location)

  ready_count = 0
  while (ready_count < num):
    ready_count = 0;
    instances = ec2.get_only_instances()
    for inst in instances:
      if inst.state == 'running':
        ready_count += 1
    print "-> number of ready instances: " + str(ready_count) + " (out of " + str(num) + ") ..."
    time.sleep(10)



def start_instances(location):

  ec2 = boto.ec2.connect_to_region()

  instances = ec2.get_only_instances()
  for inst in instances:
    ec2.start_instances(instance_ids=[inst.id])
 


def stop_instances(location):

  ec2 = boto.ec2.connect_to_region(location)

  instances = ec2.get_only_instances()
  for inst in instances:
    ec2.stop_instances(instance_ids=[inst.id])
    

def terminate_instances(location):

  ec2 = boto.ec2.connect_to_region(location)

  instances = ec2.get_only_instances()
  for inst in instances:
    ec2.terminate_instances(instance_ids=[inst.id])
    

def get_ip_addresses(location):

  ec2 = boto.ec2.connect_to_region(location)

  ip_list = []
  instances = ec2.get_only_instances()
  for inst in instances:
    if inst.state == 'running':
      ip_list.append(inst.ip_address)
  return ip_list
    

def get_dns_names(location):

  ec2 = boto.ec2.connect_to_region(location)

  dns_list = []
  instances = ec2.get_only_instances()
  for inst in instances:
    if inst.state == 'running':
      dns_list.append(inst.public_dns_name)
  return dns_list
    

