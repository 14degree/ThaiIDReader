import QtQuick 2.7
import QtQuick.Window 2.2
import QtQuick.Controls 2.2
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3
import QtQuick.Window 2.3
import QtQml.StateMachine 1.0 as DSM

Item {
    id: mainWin
    visible: true
    width: 900
    height: 480

    signal scardStartMonitor()
    signal scardStopMonitor()
    signal get_data(bool bPhoto)

    property bool bGot: false

    property int read_status: 0

    property string strPersonalTH: ""
    property string strPersonalEN: ""
    property string strAddress: ""
//    property string strCid: ""
    property string strBirthDate: ""
    property string strSex: ""

    property string strProvince: ""
    property string strProvinceCode: ""
    property string strAmphur: ""
    property string strPostcode: ""
    property string strDistrict: ""
    property string strDistrictCode: ""

    ColumnLayout {
        id: mainCol
        anchors.left: parent.left
        anchors.right: parent.right
        //anchors.horizontalCenter: parent.horizontalCenter
        //anchors.fill: parent

        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Rectangle {
                color: "lightyellow"
                height: label_3.height + 10
                Text {
                    id: label_3
                    anchors.centerIn: parent
                    text: "กรุณาเสียบบัตรที่เครื่องอ่าน"
                    font.pixelSize: 25
                }

                Layout.fillWidth: true
            }
        }

        Column {
            Layout.fillWidth: true
            //Layout.fillHeight: true
            Layout.margins: 10
            spacing: 10

            Rectangle {
                //Layout.fillWidth: true
                width: parent.width
                height: layoutCard.height + 10

                border.width: 1
                border.color: "black"

                RowLayout {
                    id: layoutCard
                    anchors.left: parent.left
                    anchors.right: parent.right

                    ColumnLayout {
                        //Layout.fillWidth: true
                        Layout.margins: 10
                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                Layout.minimumWidth: 150
                                Text {
                                    text: "หมายเลขบัตรประชาชน : "
                                    font.pixelSize: 15
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                TextField {
                                    id: txtID
                                    echoMode: TextInput.Password
                                    placeholderText: "12345678901234"
                                    Layout.minimumWidth: 500
                                    //Layout.fillWidth: true
                                    text: ""
                                    font.pixelSize: 15
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            ColumnLayout {
                                Layout.minimumWidth: 150
                                Text {
                                    text: "ชื่อ-นามสกุล : "
                                    font.pixelSize: 15
                                }
                            }
                            ColumnLayout {
                                TextField {
                                    id: txtNameTh
                                    placeholderText: "12345678901234"
                                    Layout.minimumWidth: 500
                                    //Layout.fillWidth: true
                                    text: ""
                                    font.pixelSize: 15
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            ColumnLayout {
                                Layout.minimumWidth: 150
                                Text {
                                    text: "Name - LastName : "
                                    font: txtNameTh.font
                                }
                            }
                            ColumnLayout {
                                TextField {
                                    id: txtNameEn
                                    Layout.minimumWidth: 500
                                    //Layout.fillWidth: true
                                    text: ""
                                    font: txtNameTh.font
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            ColumnLayout {
                                Layout.minimumWidth: 150
                                Text {
                                    text: "ที่อยู่ : "
                                    font: txtNameTh.font
                                }
                            }
                            ColumnLayout {
                                TextField {
                                    id: txtAddr
                                    Layout.minimumWidth: 500
                                    //Layout.fillWidth: true
                                    text: ""
                                    font.pixelSize: 15
                                }
                            }
                        }
/*
                        RowLayout {
                            Layout.fillWidth: true
                            ColumnLayout {
                                Layout.minimumWidth: 150
                                Text {
                                    text: "expire : "
                                    font: txtNameTh.font
                                }
                            }
                            ColumnLayout {
                                TextField {
                                    id: txtExpire
                                    Layout.minimumWidth: 500
                                    //Layout.fillWidth: true
                                    text: ""
                                    font.pixelSize: 15
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            ColumnLayout {
                                Layout.minimumWidth: 150
                                Text {
                                    text: "issuer : "
                                    font: txtNameTh.font
                                }
                            }
                            ColumnLayout {
                                TextField {
                                    id: txtIssuer
                                    Layout.minimumWidth: 500
                                    //Layout.fillWidth: true
                                    text: ""
                                    font.pixelSize: 15
                                }
                            }
                        }
*/
                    } // 1 st column

                    Column {
                        anchors.top: layoutCard.top
                        anchors.margins: 5
                        Image {
                            id: photo
                            width: 148
                            height: 178
                            fillMode: Image.Stretch
                            cache: false
                        }
                    }
                }
            }
        }
    }
/*
    Column {
        anchors.top: mainCol.bottom
        anchors.topMargin: 20
        Text { text: strAddress }
        Text { text: strProvince }
        Text { text: strProvinceCode }
        Text { text: strAmphur }
        Text { text: strPostcode }
        Text { text: strDistrict }
        Text { text: strDistrictCode }
    }
*/
    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        //anchors.fill: parent
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        anchors.bottomMargin: 15
        anchors.bottom: parent.bottom

        // card status
        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            //height: btnBack.height + 10
            /*
            Button {
                id: btnBack
                text: " < back "
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter

                onClicked: {
                    gotoPrevPage(register_type);
                }
            }
            */
            Text {
                id: txtCard
                text: "no card"
                anchors.right: parent.right
                anchors.rightMargin: 5
            }
        }
    }

    Connections {
        id: scardConnecion
        target: scard
        onBkkScardInserted : {
            txtCard.text = "Card Inserted"

            if(!bGot) {
                read_status = 0
                get_data(true)
                bGot = true
            }
        }

        onBkkScardRemoved : {
            read_status = 0
            txtCard.text = "Card Removed"
            bGot = false

            //clear
            txtID.text = ""
            txtNameTh.text = ""
            txtNameEn.text = ""
            txtAddr.text = ""
            //txtIssuer.text = ""
            //txtExpire.text = ""
            photo.source = ""

            strAddress = ""
            strBirthDate = ""
            //strCid = ""
            strPersonalTH = ""
            strPersonalEN = ""

        }

        onScardGotCitizenID : {
            // split
            txtID.text = cid

//            strCid = cid

            read_status++
        }

        onScardGotPersonalInfo : {
            var strpid = pinfo.trim()//.slice(0,-3)
            var strTh = strpid.substr(0,100)
            var strEn = strpid.substr(100,100)
            var strThList = strTh.split(/#/)
            var strEnList = strEn.split(/#/)
            txtNameTh.text = strThList.join(' ').trim()
            txtNameEn.text = strEnList.join(' ').trim()

            strSex = strpid.substr(208,1)
            strPersonalTH = strTh
            strPersonalEN = strEn

            var year = parseInt(strpid.substr(200, 4)) - 543
            var month = strpid.substr(204, 2)
            var day = strpid.substr(206, 2)

            strBirthDate = year.toString() + '-' + month + '-' + day

            read_status++
        }

        onScardGotAddress : {
            var straddr = addr.trim().slice(0, -3)
            var strlist = straddr.split(/[#|]+/);
            txtAddr.text = strlist.join(' ').trim()
            txtAddr.cursorPosition = 0

            strAddress = straddr

            read_status++
        }

        onScardGotCardIssueExpire : {
            var strIssExp = exp.slice(0, -3).trim()
            var year = exp.substr(8, 4);
            var month = exp.substr(12, 2);
            var day = exp.substr(14, 2);
            //txtExpire.text = day + '/' + month + '/' + year
        }

        onScardGotCardIssuer : {
            var strIssuer = iss.slice(0, -3).trim()
            //txtIssuer.text = strIssuer
        }

        onScardGotPhoto : {
            photo.source = "file:///" + applicationDirPath + "/photo.jpg"

            read_status++
        }
    }

    onRead_statusChanged: {
        if(read_status == 4) {

            var strThList = strPersonalTH.split(/#/)
            var strEnList = strPersonalEN.split(/#/)
            var strAddrList = strAddress.split(/#/)
/*
            console.log(strThList)
            console.log(strAddrList)

            var province = provincesCSV.search(0, 2, strAddrList[7].toString())
            var amphur = amphursCSV.search(0, 2, strAddrList[6].toString(), 1, province_code.toString())
            var post_code = amphursCSV.search(4, 2, strAddrList[6].toString(), 1, province_code.toString())
            var district = districtsCSV.search(0, 2, strAddrList[5].toString(), 1, province_code.toString())

            console.log('province' + ' ' + province + ' ' + province_code)
            console.log('amphur' + ' '  + amphur)
            console.log('post code' + ' ' + post_code)
            console.log('district' + ' ' + district)

            var province_code = provincesCSV.search(1, 2, strAddrList[7].toString())

            strProvince = strAddrList[7].toString()//provincesCSV.search(0, 2, strAddrList[7].toString())
            if (strProvince.search('จังหวัด') > -1) {
                // remove it
                strProvince = strProvince.substring(7)
            }
            strProvince = provincesCSV.search(0, 2, strProvince)

            strProvinceCode = provincesCSV.search(1, 2, strProvince)

            strAmphur = strAddrList[6].toString()//amphursCSV.search(0, 2, strAddrList[6].toString(), 1, province_code.toString())
            if (strAmphur.search('อำเภอ') > -1) {
                strAmphur = strAmphur.substring(5)
            }
            var strAmphurCode = amphursCSV.search(0, 2, strAmphur, 1, strProvinceCode)

            strPostcode = amphursCSV.search(4, 2, strAmphur, 1, strProvinceCode)

            strDistrict = strAddrList[5].toString()//districtsCSV.search(0, 2, strAddrList[5].toString(), 1, province_code.toString())
            if (strDistrict.search('ตำบล') > -1) {
                strDistrict = strDistrict.substring(4)
            }
            strDistrictCode = districtsCSV.search(0, 2, strDistrict, 1, strProvinceCode)
*/

        }
    }

    function startMonitor () {
        console.log("start monitor")
        scardStartMonitor()
    }
}
