#!/usr/bin/env python
import sys
from lxml import etree as ET
from xml.dom import minidom

#Project release setting
remote_git = "http://git.novatek.com.tw/scm/ivot_bsp/"
remote_name = "nvt_ivot_bsp"
#remote_branch = "master"

if len(sys.argv) < 6:
	print "\rUsage:\n\t", sys.argv[0], "manifest_file_name remote_branch na51023_bsp_id na51023_alg_rel_id na51023_testkit_id\n"
	sys.exit(1)

# the sequence will follow manifest.xml order
print 'GIT hash:'
manifest_file_name = sys.argv[1]
remote_branch = sys.argv[2]
na51023_bsp = sys.argv[3]
na51023_alg_rel = sys.argv[4]
na51023_testkit = sys.argv[5]
print "\tna51023_bsp: ", na51023_bsp
print "\tna51023_alg_rel: ", na51023_alg_rel
print "\tna51023_testkit: ", na51023_testkit


xml_root = ET.Element('manifest')
xml_remote = ET.SubElement(xml_root, 'remote', fetch=remote_git, name=remote_name, review="")
xml_default = ET.SubElement(xml_root, 'default', remote=remote_name, revision=remote_branch)
xml_content = ET.SubElement(xml_root, 'project', group="default", name="na51023_bsp", path="na51023_bsp", revision=na51023_bsp)
xml_content = ET.SubElement(xml_root, 'project', group="default", name="na51023_alg_rel", path="na51023_bsp/uitron/Alg", revision=na51023_alg_rel)
xml_content = ET.SubElement(xml_root, 'project', group="default", name="na51023_testkit", path="na51023_bsp/uitron/Project/TestKit", revision=na51023_testkit)

#print ET.tostring(root, pretty_print=True, xml_declaration=True)
tree = ET.ElementTree(xml_root)
tree.write(manifest_file_name.lower(), encoding="UTF-8", pretty_print=True, xml_declaration=True)
