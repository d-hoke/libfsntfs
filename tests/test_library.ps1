# Tests C library functions and types.
#
# Version: 20181221

$ExitSuccess = 0
$ExitFailure = 1
$ExitIgnore = 77

$LibraryTests = "attribute attribute_list attribute_list_entry bitmap_values buffer_data_handle cluster_block cluster_block_data cluster_block_stream cluster_block_vector compressed_block compressed_block_data_handle compressed_block_vector compressed_data_handle compression compression_unit_data_handle compression_unit_descriptor data_extent data_run data_stream directory_entries_tree directory_entry error file_entry file_name_attribute file_name_values file_system fixup_values index index_entry index_entry_header index_entry_vector index_node index_node_header index_root_header index_value io_handle logged_utility_stream_values mft mft_attribute mft_entry mft_entry_header name notify object_identifier_values path_hint reparse_point_attribute reparse_point_values sds_index_value security_descriptor_index security_descriptor_index_value security_descriptor_values standard_information_values txf_data_values usn_change_journal volume_header volume_information_attribute volume_information_values volume_name_attribute volume_name_values"
$LibraryTestsWithInput = "mft_metadata_file support volume"

$InputGlob = "*"

Function GetTestProfileDirectory
{
	param( [string]$TestInputDirectory, [string]$TestProfile )

	$TestProfileDirectory = "${TestInputDirectory}\.${TestProfile}"

	If (-Not (Test-Path -Path ${TestProfileDirectory} -PathType "Container"))
	{
		New-Item -ItemType "directory" -Path ${TestProfileDirectory}
	}
	Return ${TestProfileDirectory}
}

Function GetTestSetDirectory
{
	param( [string]$TestProfileDirectory, [string]$TestSetInputDirectory )

	$TestSetDirectory = "${TestProfileDirectory}\${TestSetInputDirectory.Basename}"

	If (-Not (Test-Path -Path ${TestSetDirectory} -PathType "Container"))
	{
		New-Item -ItemType "directory" -Path ${TestSetDirectory}
	}
	Return ${TestSetDirectory}
}

Function GetTestToolDirectory
{
	$TestToolDirectory = ""

	ForEach (${VSDirectory} in "msvscpp vs2008 vs2010 vs2012 vs2013 vs2015 vs2017" -split " ")
	{
		ForEach (${VSConfiguration} in "Release VSDebug" -split " ")
		{
			ForEach (${VSPlatform} in "Win32 x64" -split " ")
			{
				$TestToolDirectory = "..\${VSDirectory}\${VSConfiguration}\${VSPlatform}"

				If (Test-Path ${TestToolDirectory})
				{
					Return ${TestToolDirectory}
				}
			}
			$TestToolDirectory = "..\${VSDirectory}\${VSConfiguration}"

			If (Test-Path ${TestToolDirectory})
			{
				Return ${TestToolDirectory}
			}
		}
	}
	Return ${TestToolDirectory}
}

Function ReadIgnoreList
{
	param( [string]$TestProfileDirectory )

	$IgnoreFile = "${TestProfileDirectory}\ignore"
	$IgnoreList = ""

	If (Test-Path -Path ${IgnoreFile} -PathType "Leaf")
	{
		$IgnoreList = Get-Content -Path ${IgnoreFile} | Where {$_ -notmatch '^#.*'}
	}
	Return $IgnoreList
}

Function RunTest
{
	param( [string]$TestType )

	$TestDescription = "Testing: ${TestName}"
	$TestExecutable = "${TestToolDirectory}\fsntfs_test_${TestName}.exe"

	$Output = Invoke-Expression ${TestExecutable}
	$Result = ${LastExitCode}

	If (${Result} -ne ${ExitSuccess})
	{
		Write-Host ${Output} -foreground Red
	}
	Write-Host "${TestDescription} " -nonewline

	If (${Result} -ne ${ExitSuccess})
	{
		Write-Host " (FAIL)"
	}
	Else
	{
		Write-Host " (PASS)"
	}
	Return ${Result}
}

Function RunTestWithInput
{
	param( [string]$TestType )

	$TestDescription = "Testing: ${TestName}"
	$TestExecutable = "${TestToolDirectory}\fsntfs_test_${TestName}.exe"

	$TestProfileDirectory = GetTestProfileDirectory "input" "libfsntfs"

	$IgnoreList = ReadIgnoreList ${TestProfileDirectory}

	$Result = ${ExitSuccess}

	ForEach ($TestSetInputDirectory in Get-ChildItem -Path "input" -Exclude ".*")
	{
		If (-Not (Test-Path -Path ${TestSetInputDirectory} -PathType "Container"))
		{
			Continue
		}
		If (${TestSetInputDirectory} -Contains ${IgnoreList})
		{
			Continue
		}
		$TestSetDirectory = GetTestSetDirectory ${TestProfileDirectory} ${TestSetInputDirectory}

		If (Test-Path -Path "${TestSetDirectory}\files" -PathType "Leaf")
		{
			$InputFiles = Get-Content -Path "${TestSetDirectory}\files" | Where {$_ -ne ""}
		}
		Else
		{
			$InputFiles = Get-ChildItem -Path ${TestSetInputDirectory} -Include ${InputGlob}
		}
		ForEach ($InputFile in ${InputFiles})
		{
			# TODO: add test option support
			$Output = Invoke-Expression ${TestExecutable}
			$Result = ${LastExitCode}

			If (${Result} -ne ${ExitSuccess})
			{
				Break
			}
		}
		If (${Result} -ne ${ExitSuccess})
		{
			Break
		}
	}
	If (${Result} -ne ${ExitSuccess})
	{
		Write-Host ${Output} -foreground Red
	}
	Write-Host "${TestDescription} " -nonewline

	If (${Result} -ne ${ExitSuccess})
	{
		Write-Host " (FAIL)"
	}
	Else
	{
		Write-Host " (PASS)"
	}
	Return ${Result}
}

$TestToolDirectory = GetTestToolDirectory

If (-Not (Test-Path ${TestToolDirectory}))
{
	Write-Host "Missing test tool directory." -foreground Red

	Exit ${ExitFailure}
}

$Result = ${ExitIgnore}

Foreach (${TestName} in ${LibraryTests} -split " ")
{
	# Split will return an array of a single empty string when LibraryTests is empty.
	If (-Not (${TestName}))
	{
		Continue
	}
	$Result = RunTest ${TestName}

	If (${Result} -ne ${ExitSuccess})
	{
		Break
	}
}

Foreach (${TestName} in ${LibraryTestsWithInput} -split " ")
{
	# Split will return an array of a single empty string when LibraryTestsWithInput is empty.
	If (-Not (${TestName}))
	{
		Continue
	}
	If (Test-Path -Path "input" -PathType "Container")
	{
		$Result = RunTestWithInput ${TestName}
	}
	Else
	{
		$Result = RunTest ${TestName}
	}
	If (${Result} -ne ${ExitSuccess})
	{
		Break
	}
}

Exit ${Result}

