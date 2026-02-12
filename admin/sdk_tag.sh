#!/bin/bash
if [ -f ${LINUX_BUILD_TOP}/admin/json_parser.sh ]; then
	source ${LINUX_BUILD_TOP}/admin/json_parser.sh
	if [ $? -eq 1 ]; then
		echo -e "\e[1;31mStop tagging, we can't find json config file.\e[0m"
		exit 1
	fi
else
	echo -e "\e[1;31mStop tagging, we can't find necessary procedures.\e[0m"
	exit 1
fi
#############################################################################

TAG_NAME="REL_${SDK_PACK_COMP_NAME}"
if [ ! -z $1 ]; then
	TAG_NAME="${TAG_NAME}_$1"
fi
MANIFEST_NAME="${TAG_NAME}.xml"
MANIFEST_NAME=`echo $MANIFEST_NAME | awk '{print tolower($0)}'`

#echo "ChangeList:"
#cat ${LINUX_BUILD_TOP}/admin/ChangeList | sed -n '4,6p'
echo -e "\e[1;46mCurrent codebase:\e[0m"
repo forall -c 'echo -e "\e[1;34m$REPO_PROJECT\e[0m"; git log HEAD -n 1; echo -e "===============================================\n"'
echo -e "\e[1;46mTAG name:\e[0m"
echo -e "\t\e[1;32m$TAG_NAME\e[0m\n"
echo -e "\e[1;46mManifest name:\e[0m"
echo -e "\t\e[1;32m$MANIFEST_NAME\e[0m\n"
echo -e "\e[1;33m\rAre you sure to tag? (y/n)\r\e[0m"
read answer
if [ $answer != "y" ]; then
	exit 0;
fi
echo -e "\e[1;42m\rLocal branch tagging\r\e[0m"
repo forall -c 'if [ ! -z "`git tag -l | grep '$TAG_NAME'`" ]; then git tag -d '$TAG_NAME'; fi; git tag -a '$TAG_NAME' -m '$TAG_NAME''
echo -e "\e[1;42m\rRemote branch tagging\r\e[0m"
repo forall -c 'echo -e "\e[1;44m$REPO_PATH\e[0m"; if [ ! -z "`git ls-remote --tags | grep '$TAG_NAME'`" ]; then git push $REPO_REMOTE :'$TAG_NAME'; fi'
repo forall -c 'echo -e "\e[1;44m$REPO_PATH\e[0m"; git push $REPO_REMOTE '$TAG_NAME':'$TAG_NAME''
echo -e "\e[1;42m\rmanifest xml generation\r\e[0m"
git_hash_id=`repo forall -c 'git log --pretty=format:"%H " -1'`
python ${LINUX_BUILD_TOP}/admin/gen_manifest_xml.py  $MANIFEST_NAME $NVT_PACK_BRANCH $git_hash_id
mv $MANIFEST_NAME ${LINUX_BUILD_TOP}/../.repo/manifests
echo -e "\e[1;31mPlease commit manifest xml for this tagging version => \e[0m\e[1;33m${LINUX_BUILD_TOP}/../.repo/manifests/$MANIFEST_NAME\e[0m"
