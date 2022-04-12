#!/bin/bash


help() {
	# Display help
	echo "$0 {OPTIONS}"
	echo ""
	echo "Trace process simulation program."
	echo ""
	echo "OPTIONS:"
	echo "    -h            Print this help."
	echo "    -V            Print version."
	echo "    -v            Print verbose information."
	echo "    -r[run]       Set run number, necassary."
	echo "    -m            Run in multi-thread mode."
	echo "    -p[name]      Set project name, necassary."
	echo "    -s[rate]      Set sampling rate, 100 or 250 MHz, default is 100MHz."
	echo "    -f[strip]     Set front strip count, default is 16."
	echo "    -b[strip]     Set back strip count, default is 16."
	echo "    -w[ns]        Set hit time window in ns, default is 50."
	echo "    -z[point]     Set the zero point of trace, default is 1000."
	echo "    -a[point]     Set the start point of trace, default is 0."
	echo "    -l[length]    Set the trace length, default is 5000."
	echo "    -g[flag]      Set the run flag, all, slow, fast, cfd, sepearted by ':', default is all."
	echo "    -e[entry]     Set the run entries, default is 0(all entries)."
	echo "    -t[type]      Set the simulation type:"
	echo "                    cfg, base, tree, default is tree."
	echo "    -F[filter]    Set the filter algorithm, seperated by ':', \"slow:fast:cfd\""
	echo "                    ep(empty-filter), mw(moving-window), xa(xia-filter)"
	echo "    -P[picker]    Set the pick algorithm, seperated by ':', \"slow:fast:cfd\""
	echo "                    mx(max), tt(trapezoid-top), le(leading-edge)"
	echo "                    zcl(zero-cross-linear), zcc(zero-cross-cubic)"
	echo "                    dfl(digital-fraction-linear), dfc(digital-fraction-cubic)"
	echo ""
	echo "Produced by pwl."
	exit
}

cubic=false
rate=100
fs=16
bs=16
width=50
zeroPoint=1000
traceStart=0
traceLen=5000
simType="tree"
filterType="xa:xa:xa"
pickerType="tt:le:zcl"
runFlagStr="all"
multiThread=false
entries=0
verbose=false

while getopts ":v :h :V :m r: p: s: f: b: w: z: a: l: g: e: t: F: P:" flag;
do
	case $flag in
		h) # display help
			help
			exit;;
		V) # display version
			echo "$0 version 1.0"
			exit;;
		v) # verbose information
			verbose=true;;
		r) # set run number
			run=$OPTARG;;
		p) # set project name
			project=$OPTARG;;
		s) # set sampling rate
			rate=$OPTARG;;
		f) # set fron strip
			fs=$OPTARG;;
		b) # set back strip
			bs=$OPTARG;;
		w) # set hit time window in ns
			width=$OPTARG;;
		z) # set the zero point of the trace
			zeroPoint=$OPTARG;;
		a) # set the start point of the short trace
			traceStart=$OPTARG;;
		l) # set the length of the short trace
			traceLen=$OPTARG;;
		g) # select the run flag
			runFlagStr=$OPTARG;;
		e) # set the run entries
			entries=$OPTARG;;
		t) # set the simulator type
			simType=$OPTARG;;
		F) # set the filter type, seperated by ':'
			filterType=$OPTARG
			colonCount=$(echo "${filterType}" | awk -F ":" '{printf NF-1}')
			if [[ ${colonCount} -ne 2 ]]; then
				echo "Error option of -F ${filterType}, at least 2 ':'"
				exit
			else
				slowFilterType=${filterType%%:*}
				fastFilterType=${filterType%:*}
				fastFilterType=${fastFilterType#*:}
				cfdFilterType=${filterType##*:}
			fi;;
		P) # set the picker type, seperated by ':'
			pickerType=$OPTARG
			colonCount=$(echo "${pickerType}" | awk -F ":" '{printf NF-1}')
			if [[ ${colonCount} -ne 2 ]]; then
				echo "Error option of -F ${pickerType}, at least 2 ':'"
				exit
			else
				slowPickerType=${pickerType%%:*}
				fastPickerType=${pickerType%:*}
				fastPickerType=${fastPickerType#*:}
				cfdPickerType=${pickerType##*:}
			fi;;
		m) # run in multi-thread mode
			multiThread=true;;
		\?) # Invalid option
        	echo "Error: Invalid option"
        	help;;
	esac
done


# check parameters
# run
if [ "$run" == "" ]; then
	help
else
	rawFile=$(printf "data_R%04d.root" $run)
fi
# project
if [ "$project" == "" ]; then
	help
fi
# rate
if [[ $rate != 100 && $rate != 250 && $rate != 500 ]]; then
	echo "The sampling rate not supported: ${rate}"
	exit
fi
# fs
if [[ $fs -le 0 ]]; then
	echo "Front strip must be positive."
	exit
fi
#bs
if [[ $bs -le 0 ]]; then
	echo "Back strip must be positive."
	exit
fi
# run flag
runFlag=0
while [[ ${#runFlagStr} -ne 0 ]]; do
	str=${runFlagStr%%:*}
	runFlagStr=${runFlagStr:${#str}+1}
	# echo "str = ${str}, runFlagStr = ${runFlagStr}"
	case $str in
		all) # run in slow, fast and cfd mode
			runFlag=$(( runFlag | 0x7 ));;
		slow) # just run slow filter (energy)
			runFlag=$(( runFlag | 0x1 ));;
		fast) # just run fast filter (time)
			runFlag=$(( runFlag | 0x2 ));;
		cfd) # run cfd and fast filter (cfd time)
			runFlag=$(( runFlag | 0x4 ));;
		*) # default
			echo "Error: invalid argument of flag -g ${runFlagStr}"
			help;;
	esac
done


# filter type
# slow filter
if [[ ${slowFilterType} == "ep" || ${slowFilterType} == "" ]]; then
	slowFilterType="empty"
elif [[ ${slowFilterType} == "mw" ]]; then
	slowFilterType="mwd"
elif [[ ${slowFilterType} == "xa" ]]; then
	slowFilterType="xia"
else
	echo "Error: invalid slow filter type ${slowFilterType} (flag -F)"
	help
fi
# fast filter
if [[ ${fastFilterType} == "ep" || ${fastFilterType} == "" ]]; then
	fastFilterType="empty"
elif [[ ${fastFilterType} == "xa" ]]; then
	fastFilterType="xia"
else
	echo "Error: invalid fast filter type ${fastFilterType} (flag -F)"
	help
fi
# cfd filter
# slow filter
if [[ ${cfdFilterType} == "ep" || ${cfdFilterType} == "" ]]; then
	cfdFilterType="empty"
elif [[ ${cfdFilterType} == "xa" ]]; then
	cfdFilterType="xia"
else
	echo "Error: invalid cfd filter type ${cfdFilterType} (flag -F)"
	help
fi

# picker type
# slow picker
if [[ ${slowPickerType} == "mx" || ${slowPickerType} == "" ]]; then
	slowPickerType="max"
elif [[ ${slowPickerType} == "tt" ]]; then
	slowPickerType="trapezoid-top"
else
	echo "Error: invalid slow picker type ${slowPickerType} (flag -P)"
	help
fi
# fast picker
if [[ ${fastPickerType} == "mx" || ${fastPickerType} == "" ]]; then
	fastPickerType="max"
elif [[ ${fastPickerType} == "le" ]]; then
	fastPickerType="leading-edge"
else
	echo "Error: invalid fast picker type ${fastPickerType} (flag -P)"
	help
fi
# cfd picker
if [[ ${cfdPickerType:2:1} == "l" || ${cfdPickerType} == "" ]]; then
	cfdCubic=false
elif [[ ${cfdPickerType:2:1} == "c" ]]; then
	cfdCubic=true
else
	echo "Error: invalid cfd picker type ${cfdPickerType} (flag -P)"
	help
fi
if [[ ${cfdPickerType:0:2} == "mx" || ${cfdPickerType} == "" ]]; then
	cfdPickerType="max"
elif [[ ${cfdPickerType:0:2} == "zc" ]]; then
	cfdPickerType="zero-cross"
elif [[ ${cfdPickerType:0:2} == "df" ]]; then
	cfdPickerType="digital-fraction"
else
	echo "Error: invalid cfd picker type ${cfdPickerType} (flag -P)"
	help
fi

# simulator type
if [[ ${simType} != "cfg" && ${simType} != "base" && ${simType} != "tree" ]]; then
	echo "Error: invalid simulator type ${simType} (flag -t)"
	help
fi

# print information
echo "Project name: ${project}."
echo "Run flag: ${runFlagStr}"
echo "type        filter        picker"
echo "slow        ${slowFilterType}       ${slowPickerType}"
echo "fast        ${fastFilterType}       ${fastPickerType}"
echo "cfd         ${cfdFilterType}        ${cfdPickerType}"
echo "trace  ${traceStart}  ${traceLen}  ${zeroPoint}"


# prepare paramters
rootDir=$(pwd)
rootDir=${rootDir%bin}
dataPath=${rootDir}data/$project/
rawPath="/data/TestData/decode/"
tracePath=${dataPath}
simPath=${dataPath}Sim/
sepPath=${dataPath}
singlePath=${dataPath}Single/
timeResPath=${dataPath}TimeRes/
cachePath=${dataPath}cache/
resultPath=${dataPath}Result/
paths=(${dataPath} ${simPath} ${singlePath} ${timeResPath} ${cachePath} ${resultPath})

# check directories

echo "Check directories"
for path in ${paths[@]}
do
	if [ ! -e $path ]; then
		mkdir ${path}
		echo "mkdir ${path}"
	fi
done


# edit parameters in config.json
configFile="${rootDir}config.json"
# edit raw path and file
sed -i "/^.*RawFile.*$/c\	\"RawFile\": \"${rawFile}\"," ${configFile}
sed -i "/^.*RawPath.*$/c\	\"RawPath\": \"${rawPath}\"," ${configFile}
# edit trace path and file
sed -i "/^.*TraceFile.*/c\	\"TraceFile\": \"Trace${rate}M.root\"," ${configFile}
sed -i "/^.*TracePath.*/c\	\"TracePath\": \"${tracePath}\"," ${configFile}
# edit simulation path
sed -i "/^.*SimPath.*/c\	\"SimPath\": \"${simPath}\"," ${configFile}
# edit seperate file and path
sed -i "/^.*SepFile.*/c\	\"SepFile\": \"Trace${rate}MSep.root\"," ${configFile}
sed -i "/^.*SepPath.*/c\	\"SepPath\": \"${sepPath}\"," ${configFile}
# edit single path
sed -i "/^.*SinglePath.*/c\	\"SinglePath\": \"${singlePath}\"," ${configFile}
# edit res path
sed -i "/^.*TimeResPath.*/c\	\"TimeResPath\": \"${timeResPath}\"," ${configFile}
# edit cache path
sed -i "/^.*CachePath.*/c\	\"CachePath\": \"${cachePath}\"," ${configFile}
# edit fbw
sed -i "/^.*fbw.*/c\	\"fbw\": ${width}," ${configFile}
# edit strip counts
sed -i "/^.*Strips.*/c\	\"Strips\": [${fs}, ${bs}]," ${configFile}
# edit zero point of trace
sed -i "/^.*ZeroPoint.*/c\	\"ZeroPoint\": ${zeroPoint}," ${configFile}
# edit start point of the short trace
sed -i "/^.*TraceStart.*/c\	\"TraceStart\": ${traceStart}," ${configFile}
# edit length of the short trace
sed -i "/^.*TraceLength.*/c\	\"TraceLength\": ${traceLen}," ${configFile}
# edit run flag
sed -i "/^.*RunFlag.*/c\	\"RunFlag\": ${runFlag}," ${configFile}
# edit run entries
sed -i "/^.*Entries.*/c\	\"Entries\": ${entries}," ${configFile}
# edit simulator type
sed -i "/^.*Simulator.*/c\	\"Simulator\": \"${simType}\"," ${configFile}
# edit filter options
sed -i "/^.*SlowFilter.*/c\	\"SlowFilter\": \"${slowFilterType}\"," ${configFile}
sed -i "/^.*FastFilter.*/c\	\"FastFilter\": \"${fastFilterType}\"," ${configFile}
sed -i "/^.*CFDFilter.*/c\	\"CFDFilter\": \"${cfdFilterType}\"," ${configFile}
# edit picker options
sed -i "/^.*SlowPicker.*/c\	\"SlowPicker\": \"${slowPickerType}\"," ${configFile}
sed -i "/^.*FastPicker.*/c\	\"FastPicker\": \"${fastPickerType}\"," ${configFile}
sed -i "/^.*CFDPicker.*/c\	\"CFDPicker\": \"${cfdPickerType}\"," ${configFile}
sed -i "/^.*CFDCubic.*/c\	\"CFDCubic\": ${cfdCubic}," ${configFile}
# edit verbose
sed -i "/^.*Verbose.*/c\	\"Verbose\": ${verbose}," ${configFile}
# edit multi-thread option
sed -i "/^.*MultiThread.*/c\	\"MultiThread\": ${multiThread}," ${configFile}

# save the config file
cp ${configFile} ${dataPath}config.json

if [ $simType == 'cfg' ]; then
	exit
fi

binDir=${rootDir}main/

# begin simulation
# mapping with adapt
echo "=================================================="
echo "!!Start mapping, read file ${rawPath}${rawFile}"
echo "mapping code display below"
echo ""
sed -n "/${rate}M mapping code/{n;:a;N;/fill/!ba;p}" ../main/Adapter.cpp | head -n -1 | highlight -O ansi --syntax cpp

# ${binDir}adapt ${configFile} > ${cachePath}adapt.output			# run
${binDir}adapt ${configFile}

# simulation that calculates the cfd valud and cfd point
echo "!!Start simulation"
# for i in {1..8}
# do
# 	./sim $((i*10)) > ${cachePath}sim$i.output &
# done
# ${binDir}sim ${configFile} > ${cachePath}sim.output
${binDir}sim ${configFile}

# sepearte file without trace data
# ${binDir}seperate ${configFile} > ${cachePath}sep.output
# wait
echo "!!Start seperate"

${binDir}seperate ${configFile}
echo "waiting for simulation"

# analysis and generate single hit file
echo "!!Start single"
# for i in {1..8}
# do
# 	./single $((i*10)) > ${cachePath}single$i.output &
# done
# wait

# ${binDir}single ${configFile} > ${cachePath}single.output
${binDir}single ${configFile}
echo "waiting for single"

# fit and generate time resolution file
echo "!!Start res"
# ${binDir}tres ${configFile} > ${resultPath}result.output
${binDir}tres ${configFile} > ${resultPath}result.output
cat ${resultPath}result.output
# end simulation

# copy result
cp ${resultPath}result.output ${rootDir}result/${project}.result

echo "Finish!!"
