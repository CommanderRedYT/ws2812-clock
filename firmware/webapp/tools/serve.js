#!/usr/bin/env node
import extraDependencies from './dependencies.js';
import express from 'express';
import process from "process";

if (!process.cwd().endsWith('webapp')) {
    console.error('Please run this script from the firmware/webapp/ directory');
    process.exit(1);
}

const app = express();
const port = 3000;

app.use(express.static('src'));

extraDependencies.forEach(([path, name]) => {
    console.log(`app.use('/${name}', express.static('${path}'));`);
    app.use(`/${name}`, express.static(path));
});

// not found handler
app.use((req, res, next) => {
    res.status(404).send('404 Not Found');
    console.log(`404 ${req.url}`);
});

app.listen(port, () => {
    console.log(`Example app listening at http://localhost:${port}`);
});
