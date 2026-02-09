char settings[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<title>Advanced Settings</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
    html {
        height: 100%;
    }
    
    body {
        height: 100%;
        margin: 0;
        font-family: "Roboto", "Arial", sans-serif, "Open Sans";
        user-select: none;
        -moz-user-select: none;
        display: grid;
        grid-template: 1fr 10fr 1fr/ 1fr 4fr 1fr;
    }
    
    .form {
        grid-area: 2 / 2 / 2 / 2;
        margin-top: auto;
        margin-bottom: auto;
    }
    
    #back {
        grid-area: 1 / 1 / 1 / 4;
        background-color: #4c4c4c;
        color: white;
        transition: background-color 0.1s ease;
        overflow: hidden;
        position: relative;
        margin: 1vmin;
        border-radius: 1vmin;
    }
    
    #back:hover {
        background-color: #7a7a7a;
        cursor: pointer;
    }
    
    #backText {
        text-align: center;
        font-size: 6vmin;
        margin: 0;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        position: absolute;
    }
    
    #submit {
        cursor: pointer;
        background-color: #9df796;
        border-style: solid;
        border-color: #57d34e;
        width: 100%;
        height: 10%;
        font-size: 150%;
    }
    
    #noteText {
        margin: 0;
        color: red;
    }
    
    .botMessage {
        margin: 0;
        font-size: 20px;
    }
    
    #timer {
        font-weight: bold;
        color: red;
    }
</style>
</head>
<body>
    <div id="back" onclick="goBack()"><p id="backText">Back</p></div>
    <form class="form" action="/submit" method="POST" autocomplete="off">
        <fieldset>
            <legend>Wireless network settings</legend>
            <p id="noteText">Note: Changing network settings will prompt the device to reset.</p><br>
            <label for="SSIDbox">Network SSID:</label><br>
            <input type="text" id="SSIDbox" name="SSID" placeholder="SSID"><br><br>
            <label for="PASSbox">Network Password:</label><br>
            <input type="password" id="PASSbox" name="PASS" placeholder="Password"><br>
            <input id="passCheckbox" type="checkbox" onclick="showPass()"><label for="passCheckbox">Show Password</label><br>
        </fieldset><br>
        <fieldset>
            <legend>Automatic schedule settings</legend>
            <label for="STARTtime">Schedule start time:</label><br>
            <input type="time" id="STARTtime" name="startTime"><br><br>
            <label for="ENDtime">Schedule end time:</label><br>
            <input type="time" id="ENDtime" name="endTime">
        </fieldset><br>
        <input type="submit" value="Save settings" id="submit"><br><br>
    </form>
    <script>
        function goBack() {
            window.location.href = "/";
        };
        
        function showPass() {
            var x = document.getElementById("PASSbox");
            if (x.type === "password") {
                x.type = "text";
            } else {
                x.type = "password";
            }
        };
    </script>
</body>
</html>
)=====";
