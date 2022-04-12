const cheerio = require('cheerio/slim');

async function crawl(id = 1) {
  async function getRollById(id = 1) {
    const form_details = {
      'name': id,
      'search': 'Search ID!'
    };

    const response = await fetch('http://cydel.net/idlook.php', {
      method: 'POST',
      headers: {
        'Accept-Encoding': 'gzip, deflate',
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: new URLSearchParams(form_details)
    });
    const response_text = await response.text();
    return response_text;
  }

  async function getRollsByCharacter(character = '') {
    const form_details = {
      'name': character,
      'lets rolls!': 'Search!'
    };

    const response = await fetch('http://cydel.net/dicelook.php', {
      method: 'POST',
      headers: {
        'Accept-Encoding': 'gzip, deflate',
        'Content-Type': 'application/x-www-form-urlencoded'
      },
      body: new URLSearchParams(form_details)
    });
    const response_text = await response.text();
    return response_text;
  }

  let html_response;
  try {
    html_response = await getRollById(1);
  } catch (e) {
    console.error("Error: ", err);
    /* Done to propogate fetch errors */
    throw `Error when fetching roll with ID ${id}`
  }

  let $ = cheerio.load(html_response);

  const id_selector = 'body > center > table:nth-child(3) > tbody >' +
    'tr:nth-child(2) > td:nth-child(6)';

  /* Character will be undefined if no roll with ID was found*/
  const character = $(id_selector).text();

  if (typeof character === 'undefined') {
    throw `Error: Roll with ID ${id} not found.`
  }

  try {
    html_response = await getRollsByCharacter(character);
  } catch (e) {
    console.error("Error: ", err);
    throw `Error when fetching rolls with character ${character}`
  }

  $ = cheerio.load(html_response);

}

crawl(1);
