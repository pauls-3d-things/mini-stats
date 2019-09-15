const express = require('express')
const os = require('os');
const disk = require('diskusage');

const app = express()
const port = 3000

const toDoubleDigits = (num) => {
    return (num < 10 ? "0" : "") + Math.floor(num);
}

const timeToString = (uptime) => {
    const d = uptime / (60 * 60 * 24);
    const h = (uptime / (60 * 60)) % 24;
    const m = (uptime / 60) % 60;
    return Math.floor(d) + "d" + toDoubleDigits(h) + "h" + toDoubleDigits(m);
}

const getStats = () => {
    const loadavg = os.loadavg();

    return {
        host: os.hostname(),
        cores: os.cpus().length,
        freeMem: Math.floor((os.freemem() / os.totalmem()) * 100),
        uptime: timeToString(os.uptime()),
        load1: loadavg[0],
        load5: loadavg[1],
        load15: loadavg[2]
    }
}

app.get('/stats', (req, res) => {
    console.log("GET /stats", req.ip);
    const stats = getStats();

    disk.check("/", (err, info) => {
        if (err) {
            stats.freeDisk = 0;
        } else {
            stats.freeDisk = Math.floor((info.free / info.total) * 100);
        }
        res.send(stats);
    });

});


app.listen(port, () => console.log(`Started: http://localhost:${port}/stats`))