// htmlparser2 does not fix malformed html. The website we have
// uses a lot of malformed html including no tbody or anything.
// const htmlparser2 = require('htmlparser2');
const cheerio = require('cheerio');
const fs = require('fs');

async function crawl(id) {
  async function getRollById(id) {
    const form_details = {
      name: id,
      search: 'Search ID!',
    };

    const response = await fetch('http://cydel.net/idlook.php', {
      method: 'POST',
      headers: {
        'Accept-Encoding': 'gzip, deflate',
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: new URLSearchParams(form_details),
    });
    const response_text = await response.text();
    return response_text;
  }

  async function getRollsByCharacter(character = '') {
    const form_details = {
      name: character,
      'lets rolls!': 'Search!',
    };

    const response = await fetch('http://cydel.net/dicelook.php', {
      method: 'POST',
      headers: {
        'Accept-Encoding': 'gzip, deflate',
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: new URLSearchParams(form_details),
    });
    const response_text = await response.text();
    return response_text;
  }

  let html_response;
  let dom;
  try {
    html_response = await getRollById(id);
    // dom = htmlparser2.parseDocument(html_response);
  } catch (e) {
    console.error('Error: ', e);
    /* Done to propogate fetch errors */
    throw `Error when fetching roll with ID ${id}`;
  }

  let $ = cheerio.load(html_response);

  // const id_selector =
  //   'body > center > table > table:nth-child(4) > tr > ' +
  //   'td:nth-child(6)';
  const id_selector =
    'body > center:nth-child(1) > table:nth-child(3) > tbody:nth-child(1) > ' +
    'tr:nth-child(2) > td:nth-child(6)';

  /* Character will be undefined if no roll with ID was found*/
  const character = $(id_selector).html();
  let tag_included = $(id_selector).get(0).childNodes.length == 2;

  if (typeof character === 'undefined') {
    throw `Error: Roll with ID ${id} not found.`;
  }

  try {
    html_response = await getRollsByCharacter(character);
    // dom = htmlparser2.parseDocument(html_response);
  } catch (e) {
    console.error('Error: ', e);
    throw `Error when fetching rolls with character ${character}`;
  }

  // The responses are pretty malformed with single quoted attributes
  // and unclosed elements. There should technically be two tables but in the
  // raw response but it is not closed and cheerios does not pick it up.
  // const table_selector =
  //   'body > center > table > table:nth-child(4) > tr';

  const table_selector =
    'body > center:nth-child(1) > table:nth-child(' +
    (tag_included + 3) +
    ') > tbody:nth-child(1) > tr:not(:nth-child(1))';

  $ = cheerio.load(html_response);
  let tsv = '';
  $(table_selector)
    .children('td')
    .each((idx, elem) => {
      let builder;
      // This should let us skip row 0, col 0 from generating newline
      // Doing a switch case would complicate the issue a lot since we don't
      // want to fall through from col_no 7 to any other columns and
      // still want to append a tab character
      let col_no = idx - Math.floor(idx / 9) * 9;
      if (col_no === 6) {
        builder = $(elem).find('a').attr('href');
      } else {
        builder = $(elem).html();
      }

      // Short circuit and don't check builder string if not in user-controlled
      // text columns.
      if (col_no >= 5 && col_no <= 7 && builder.includes('"')) {
        builder = '"' + builder.replace(/"/g, '""') + '"';
      }

      if (col_no == 8) {
        builder += '\n';
      } else {
        builder += '\t';
      }

      tsv += builder;
    });

  return tsv;
}

crawl(1);