--TEST--
hash_copy() basic tests
--FILE--
<?php

$algos = hash_algos();

foreach ($algos as $algo) {
	$orig = hash_init($algo);
	hash_update($orig, b"I can't remember anything");
	$copy = hash_copy($orig);
	var_dump(hash_final($orig));

	var_dump(hash_final($copy));
}

foreach ($algos as $algo) {
	$orig = hash_init($algo);
	hash_update($orig, b"I can't remember anything");
	$copy = hash_copy($orig);
	var_dump(hash_final($orig));

	hash_update($copy, b"Can’t tell if this is true or dream");
	var_dump(hash_final($copy));
}

echo "Done\n";
?>
--EXPECTF--
unicode(32) "d5ac4ffd08f6a57b9bd402b8068392ff"
unicode(32) "d5ac4ffd08f6a57b9bd402b8068392ff"
unicode(32) "302c45586b53a984bd3a1237cb81c15f"
unicode(32) "302c45586b53a984bd3a1237cb81c15f"
unicode(32) "e35759f6ea35db254e415b5332269435"
unicode(32) "e35759f6ea35db254e415b5332269435"
unicode(40) "29f62a228f726cd728efa7a0ac6a2aba318baf15"
unicode(40) "29f62a228f726cd728efa7a0ac6a2aba318baf15"
unicode(56) "51fd0aa76a00b4a86103895cad5c7c2651ec7da9f4fc1e50c43ede29"
unicode(56) "51fd0aa76a00b4a86103895cad5c7c2651ec7da9f4fc1e50c43ede29"
unicode(64) "d3a13cf52af8e9390caed78b77b6b1e06e102204e3555d111dfd149bc5d54dba"
unicode(64) "d3a13cf52af8e9390caed78b77b6b1e06e102204e3555d111dfd149bc5d54dba"
unicode(96) "6950d861ace4102b803ab8b3779d2f471968233010d2608974ab89804cef6f76162b4433d6e554e11e40a7cdcf510ea3"
unicode(96) "6950d861ace4102b803ab8b3779d2f471968233010d2608974ab89804cef6f76162b4433d6e554e11e40a7cdcf510ea3"
unicode(128) "caced3db8e9e3a5543d5b933bcbe9e7834e6667545c3f5d4087b58ec8d78b4c8a4a5500c9b88f65f7368810ba9905e51f1cff3b25a5dccf76634108fb4e7ce13"
unicode(128) "caced3db8e9e3a5543d5b933bcbe9e7834e6667545c3f5d4087b58ec8d78b4c8a4a5500c9b88f65f7368810ba9905e51f1cff3b25a5dccf76634108fb4e7ce13"
unicode(32) "5f1bc5f5aeaf747574dd34a6535cd94a"
unicode(32) "5f1bc5f5aeaf747574dd34a6535cd94a"
unicode(40) "02a2a535ee10404c6b5cf9acb178a04fbed67269"
unicode(40) "02a2a535ee10404c6b5cf9acb178a04fbed67269"
unicode(64) "547d2ed85ca0a0e3208b5ecf4fc6a7fc1e64db8ff13493e4beaf11e4d71648e2"
unicode(64) "547d2ed85ca0a0e3208b5ecf4fc6a7fc1e64db8ff13493e4beaf11e4d71648e2"
unicode(80) "785a7df56858f550966cddfd59ce14b13bf4b18e7892c4c1ad91bf23bf67639bd2c96749ba29cfa6"
unicode(80) "785a7df56858f550966cddfd59ce14b13bf4b18e7892c4c1ad91bf23bf67639bd2c96749ba29cfa6"
unicode(128) "6e60597340640e621e25f975cef2b000b0c4c09a7af7d240a52d193002b0a8426fa7da7acc5b37ed9608016d4f396db834a0ea2f2c35f900461c9ac7e5604082"
unicode(128) "6e60597340640e621e25f975cef2b000b0c4c09a7af7d240a52d193002b0a8426fa7da7acc5b37ed9608016d4f396db834a0ea2f2c35f900461c9ac7e5604082"
unicode(32) "a92be6c58be7688dc6cf9585a47aa625"
unicode(32) "a92be6c58be7688dc6cf9585a47aa625"
unicode(40) "a92be6c58be7688dc6cf9585a47aa62535fc2482"
unicode(40) "a92be6c58be7688dc6cf9585a47aa62535fc2482"
unicode(48) "a92be6c58be7688dc6cf9585a47aa62535fc2482e0e5d12c"
unicode(48) "a92be6c58be7688dc6cf9585a47aa62535fc2482e0e5d12c"
unicode(32) "32fb748ef5a36ca222511bcb99b044ee"
unicode(32) "32fb748ef5a36ca222511bcb99b044ee"
unicode(40) "32fb748ef5a36ca222511bcb99b044ee1d740bf3"
unicode(40) "32fb748ef5a36ca222511bcb99b044ee1d740bf3"
unicode(48) "32fb748ef5a36ca222511bcb99b044ee1d740bf300593703"
unicode(48) "32fb748ef5a36ca222511bcb99b044ee1d740bf300593703"
unicode(64) "fbe88daa74c89b9e29468fa3cd3a657d31845e21bb58dd3f8d806f5179a85c26"
unicode(64) "fbe88daa74c89b9e29468fa3cd3a657d31845e21bb58dd3f8d806f5179a85c26"
unicode(64) "fbe88daa74c89b9e29468fa3cd3a657d31845e21bb58dd3f8d806f5179a85c26"
unicode(64) "fbe88daa74c89b9e29468fa3cd3a657d31845e21bb58dd3f8d806f5179a85c26"
unicode(64) "5820c7c4a0650587538b30ef4099f2b5993069758d5c847a552e6ef7360766a5"
unicode(64) "5820c7c4a0650587538b30ef4099f2b5993069758d5c847a552e6ef7360766a5"
unicode(8) "28097c6f"
unicode(8) "28097c6f"
unicode(8) "e5cfc160"
unicode(8) "e5cfc160"
unicode(8) "69147a4e"
unicode(8) "69147a4e"
unicode(32) "86362472c8895e68e223ef8b3711d8d9"
unicode(32) "86362472c8895e68e223ef8b3711d8d9"
unicode(40) "fabdf6905f3ba18a3c93d6a16b91e31f7222a7a4"
unicode(40) "fabdf6905f3ba18a3c93d6a16b91e31f7222a7a4"
unicode(48) "e05d0ff5723028bd5494f32c0c2494cd0b9ccf7540af7b47"
unicode(48) "e05d0ff5723028bd5494f32c0c2494cd0b9ccf7540af7b47"
unicode(56) "56b196289d8de8a22296588cf90e5b09cb6fa1b01ce8e92bca40cae2"
unicode(56) "56b196289d8de8a22296588cf90e5b09cb6fa1b01ce8e92bca40cae2"
unicode(64) "ff4d7ab0fac2ca437b945461f9b62fd16e71e9103524d5d140445a00e3d49239"
unicode(64) "ff4d7ab0fac2ca437b945461f9b62fd16e71e9103524d5d140445a00e3d49239"
unicode(32) "ee44418e0195a0c4a35d112722919a9c"
unicode(32) "ee44418e0195a0c4a35d112722919a9c"
unicode(40) "f320cce982d5201a1ccacc1c5ff835a258a97eb1"
unicode(40) "f320cce982d5201a1ccacc1c5ff835a258a97eb1"
unicode(48) "a96600107463e8e97a7fe6f260d9bf4f4587a281caafa6db"
unicode(48) "a96600107463e8e97a7fe6f260d9bf4f4587a281caafa6db"
unicode(56) "7147c9e1c1e67b942da3229f59a1ab18f121f5d7f5765ca88bc9f200"
unicode(56) "7147c9e1c1e67b942da3229f59a1ab18f121f5d7f5765ca88bc9f200"
unicode(64) "82fec42679ed5a77a841962827b88a9cddf7d677736e50bc81f1a14b99f06061"
unicode(64) "82fec42679ed5a77a841962827b88a9cddf7d677736e50bc81f1a14b99f06061"
unicode(32) "8d0b157828328ae7d34d60b4b60c1dab"
unicode(32) "8d0b157828328ae7d34d60b4b60c1dab"
unicode(40) "54dab5e10dc41503f9b8aa32ffe3bab7cf1da8a3"
unicode(40) "54dab5e10dc41503f9b8aa32ffe3bab7cf1da8a3"
unicode(48) "7d91265a1b27698279d8d95a5ee0a20014528070bf6415e7"
unicode(48) "7d91265a1b27698279d8d95a5ee0a20014528070bf6415e7"
unicode(56) "7772b2e22f2a3bce917e08cf57ebece46bb33168619a776c6f2f7234"
unicode(56) "7772b2e22f2a3bce917e08cf57ebece46bb33168619a776c6f2f7234"
unicode(64) "438a602cb1a761f7bd0a633b7bd8b3ccd0577b524d05174ca1ae1f559b9a2c2a"
unicode(64) "438a602cb1a761f7bd0a633b7bd8b3ccd0577b524d05174ca1ae1f559b9a2c2a"
unicode(32) "d5ac4ffd08f6a57b9bd402b8068392ff"
unicode(32) "5c36f61062d091a8324991132c5e8dbd"
unicode(32) "302c45586b53a984bd3a1237cb81c15f"
unicode(32) "1d4196526aada3506efb4c7425651584"
unicode(32) "e35759f6ea35db254e415b5332269435"
unicode(32) "f255c114bd6ce94aad092b5141c00d46"
unicode(40) "29f62a228f726cd728efa7a0ac6a2aba318baf15"
unicode(40) "a273396f056554dcd491b5dea1e7baa3b89b802b"
unicode(56) "51fd0aa76a00b4a86103895cad5c7c2651ec7da9f4fc1e50c43ede29"
unicode(56) "1aee028400c56ceb5539625dc2f395abf491409336ca0f3e177a50e2"
unicode(64) "d3a13cf52af8e9390caed78b77b6b1e06e102204e3555d111dfd149bc5d54dba"
unicode(64) "268e7f4cf88504a53fd77136c4c4748169f46ff7150b376569ada9c374836944"
unicode(96) "6950d861ace4102b803ab8b3779d2f471968233010d2608974ab89804cef6f76162b4433d6e554e11e40a7cdcf510ea3"
unicode(96) "0d44981d04bb11b1ef75d5c2932bd0aa2785e7bc454daac954d77e2ca10047879b58997533fc99650b20049c6cb9a6cc"
unicode(128) "caced3db8e9e3a5543d5b933bcbe9e7834e6667545c3f5d4087b58ec8d78b4c8a4a5500c9b88f65f7368810ba9905e51f1cff3b25a5dccf76634108fb4e7ce13"
unicode(128) "28d7c721433782a880f840af0c3f3ea2cad4ef55de2114dda9d504cedeb110e1cf2519c49e4b5da3da4484bb6ba4fd1621ceadc6408f4410b2ebe9d83a4202c2"
unicode(32) "5f1bc5f5aeaf747574dd34a6535cd94a"
unicode(32) "f95f5e22b8875ee0c48219ae97f0674b"
unicode(40) "02a2a535ee10404c6b5cf9acb178a04fbed67269"
unicode(40) "900d615c1abe714e340f4ecd6a3d65599fd30ff4"
unicode(64) "547d2ed85ca0a0e3208b5ecf4fc6a7fc1e64db8ff13493e4beaf11e4d71648e2"
unicode(64) "b9799db40d1af5614118c329169cdcd2c718db6af03bf945ea7f7ba72b8e14f4"
unicode(80) "785a7df56858f550966cddfd59ce14b13bf4b18e7892c4c1ad91bf23bf67639bd2c96749ba29cfa6"
unicode(80) "d6d12c1fca7a9c4a59c1be4f40188e92a746a035219e0a6ca1ee53b36a8282527187f7dffaa57ecc"
unicode(128) "6e60597340640e621e25f975cef2b000b0c4c09a7af7d240a52d193002b0a8426fa7da7acc5b37ed9608016d4f396db834a0ea2f2c35f900461c9ac7e5604082"
unicode(128) "e8c6a921e7d8eac2fd21d4df6054bb27a02321b2beb5b01b6f88c40706164e64d67ec97519bf76c8af8df896745478b78d42a0159f1a0db16777771fd9d420dc"
unicode(32) "a92be6c58be7688dc6cf9585a47aa625"
unicode(32) "dc80d448032c9da9f1e0262985353c0f"
unicode(40) "a92be6c58be7688dc6cf9585a47aa62535fc2482"
unicode(40) "dc80d448032c9da9f1e0262985353c0fe37e9551"
unicode(48) "a92be6c58be7688dc6cf9585a47aa62535fc2482e0e5d12c"
unicode(48) "dc80d448032c9da9f1e0262985353c0fe37e9551165c82e1"
unicode(32) "32fb748ef5a36ca222511bcb99b044ee"
unicode(32) "e5c4212432c0e266e581d4ee6a8e20a9"
unicode(40) "32fb748ef5a36ca222511bcb99b044ee1d740bf3"
unicode(40) "e5c4212432c0e266e581d4ee6a8e20a9d0d944e3"
unicode(48) "32fb748ef5a36ca222511bcb99b044ee1d740bf300593703"
unicode(48) "e5c4212432c0e266e581d4ee6a8e20a9d0d944e34804b0c4"
unicode(64) "fbe88daa74c89b9e29468fa3cd3a657d31845e21bb58dd3f8d806f5179a85c26"
unicode(64) "614ca924864fa0e8fa309aa0944e047d5edbfd4964a35858f4d8ec66a0fb88b0"
unicode(64) "fbe88daa74c89b9e29468fa3cd3a657d31845e21bb58dd3f8d806f5179a85c26"
unicode(64) "614ca924864fa0e8fa309aa0944e047d5edbfd4964a35858f4d8ec66a0fb88b0"
unicode(64) "5820c7c4a0650587538b30ef4099f2b5993069758d5c847a552e6ef7360766a5"
unicode(64) "a00961e371287c71c527a41c14564f13b6ed12ac7cd9d5f5dfb3542a25e28d3b"
unicode(8) "28097c6f"
unicode(8) "471714d9"
unicode(8) "e5cfc160"
unicode(8) "59f8d3d2"
unicode(8) "69147a4e"
unicode(8) "3ee63999"
unicode(32) "86362472c8895e68e223ef8b3711d8d9"
unicode(32) "ebeeeb05c18af1e53d2d127b561d5e0d"
unicode(40) "fabdf6905f3ba18a3c93d6a16b91e31f7222a7a4"
unicode(40) "f1a2c9604fb40899ad502abe0dfcec65115c8a9a"
unicode(48) "e05d0ff5723028bd5494f32c0c2494cd0b9ccf7540af7b47"
unicode(48) "d3a7315773a326678208650ed02510ed96cd488d74cd5231"
unicode(56) "56b196289d8de8a22296588cf90e5b09cb6fa1b01ce8e92bca40cae2"
unicode(56) "6d7132fabc83c9ab7913748b79ecf10e25409569d3ed144177f46731"
unicode(64) "ff4d7ab0fac2ca437b945461f9b62fd16e71e9103524d5d140445a00e3d49239"
unicode(64) "7a469868ad4b92891a3a44524c58a2b8d0f3bebb92b4cf47d19bc6aba973eb95"
unicode(32) "ee44418e0195a0c4a35d112722919a9c"
unicode(32) "6ecddb39615f43fd211839287ff38461"
unicode(40) "f320cce982d5201a1ccacc1c5ff835a258a97eb1"
unicode(40) "bcd2e7821723ac22e122b8b7cbbd2daaa9a862df"
unicode(48) "a96600107463e8e97a7fe6f260d9bf4f4587a281caafa6db"
unicode(48) "ae74619a88dcec1fbecde28e27f009a65ecc12170824d2cd"
unicode(56) "7147c9e1c1e67b942da3229f59a1ab18f121f5d7f5765ca88bc9f200"
unicode(56) "fdaba6563f1334d40de24e311f14b324577f97c3b78b9439c408cdca"
unicode(64) "82fec42679ed5a77a841962827b88a9cddf7d677736e50bc81f1a14b99f06061"
unicode(64) "289a2ba4820218bdb25a6534fbdf693f9de101362584fdd41e32244c719caa37"
unicode(32) "8d0b157828328ae7d34d60b4b60c1dab"
unicode(32) "ffa7993a4e183b245263fb1f63e27343"
unicode(40) "54dab5e10dc41503f9b8aa32ffe3bab7cf1da8a3"
unicode(40) "375ee5ab3a9bd07a1dbe5d071e07b2afb3165e3b"
unicode(48) "7d91265a1b27698279d8d95a5ee0a20014528070bf6415e7"
unicode(48) "c650585f93c6e041e835caedc621f8c42d8bc6829fb76789"
unicode(56) "7772b2e22f2a3bce917e08cf57ebece46bb33168619a776c6f2f7234"
unicode(56) "bc674d465a822817d939f19b38edde083fe5668759836c203c56e3e4"
unicode(64) "438a602cb1a761f7bd0a633b7bd8b3ccd0577b524d05174ca1ae1f559b9a2c2a"
unicode(64) "da70ad9bd09ed7c9675329ea2b5279d57761807c7aeac6340d94b5d494809457"
Done
