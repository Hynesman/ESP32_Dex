// HTML for the upload page
const char* uploadPage = R"(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>OTA Update</title>
<style>
.rounded-logo {
    height: 200px;
    width: 200px;
    border-radius: 10px;
}
body {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    font-family: 'Arial', sans-serif;
    margin: 0;
    height: 100vh;
    text-align: center;
    background-color: #87CEEB; /* Soft Sky Blue */
    color: #204056; /* Darker shade for text */
}
h2 {
    font-size: 24px;
    color: #204056; /* Ensuring readability */
}
#progressBarContainer {
    width: 80%; /* Adjusted width */
    background-color: #B0E0E6; /* Powder Blue for contrast */
    border-radius: 25px;
    padding: 3px; /* Slight padding around progress bar */
    box-shadow: 0 2px 4px rgba(0,0,0,0.2); /* Soft shadow for depth */
}
#progressBar {
    width: 0%;
    height: 30px;
    background-color: #5DADE2; /* Stronger Blue */
    border-radius: 20px;
    color: white; /* Text color */
    font-weight: bold; /* Bold text */
}
form {
    background-color: #D6EAF8; /* Lighter blue */
    padding: 20px;
    border-radius: 10px;
    box-shadow: 0 2px 5px rgba(0,0,0,0.1); /* Soft shadow for form */
    margin: 20px 0; /* Space around form */
}
input[type="file"] {
    font-size: 16px;
    margin: 10px 0; /* Space around input */
}
input[type="submit"] {
    background-color: #3498DB; /* Vivid blue */
    color: white;
    border: none;
    padding: 10px 15px;
    font-size: 18px;
    border-radius: 5px;
    box-shadow: 0 2px 4px rgba(0,0,0,0.2); /* Soft shadow for button */
}
input[type="submit"]:hover {
    background-color: #2E86C1; /* Darken button on hover */
}
</style>
</head>
<body>
<h2>Glucose Follower Firmware Update</h2>
<img src="/logoGPT.png" alt="Logo" class="rounded-logo" />
<form method="POST" action="/update" enctype="multipart/form-data" onsubmit="return onSubmit();">
    <input type="file" name="update">
    <input type="submit" value="Update Firmware">
</form>
<div id="progressBarContainer">
    <div id="progressBar">0%</div>
</div>
<script>
function onSubmit() {
    var form = document.querySelector('form');
    var request = new XMLHttpRequest();
    request.upload.addEventListener('progress', function(e) {
        var percent = e.loaded / e.total * 100;
        var progressBar = document.getElementById('progressBar');
        progressBar.style.width = percent.toFixed(2) + '%';
        progressBar.innerHTML = percent.toFixed(2) + '%';
    });
    request.addEventListener('load', function(e) {
        document.body.innerHTML = request.responseText;
    });
    request.open(form.method, form.action);
    request.send(new FormData(form));
    return false; // Prevent traditional form submission
}

</script>
</body>
</html>
)";



// HTML for the success page
const char* successPage = R"(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Update Success</title>
</head>
<body>
<h2>Update Successful!</h2>
<p>The firmware has been updated successfully, and the device will now restart.</p>
</body>
</html>
)";

// HTML for the failed page
const char* failedPage = R"(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Update Failed</title>
</head>
<body>
<h2>Update Failed!</h2>
<p>There was an error updating the firmware. Please try again.</p>
</body>
</html>
)";